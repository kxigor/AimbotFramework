#include <windows.h>

#include <afw/esp/esp.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/hook/vtable.hpp>
#include <afw/render/overlay.hpp>
#include <afw/render/present_hook.hpp>
#include <afw/render/render.hpp>
#include <afw/win/scoped_handle.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <expected>
#include <system_error>

namespace {

constexpr std::size_t kMessageChars{256};
constexpr std::size_t kSuffixChars{5};
constexpr std::uint64_t kStatusInterval{1800};

void Log(const char* text) {
  OutputDebugStringA(text);

  HMODULE module{nullptr};
  if (GetModuleHandleExW(
          /*dwFlags      = */ GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          /*lpModuleName = */ reinterpret_cast<PCWSTR>(&Log),
          /*phModule     = */ &module) == FALSE) {
    return;
  }

  std::array<WCHAR, MAX_PATH> path{};
  const DWORD kLength{GetModuleFileNameW(
      /*hModule    = */ module,
      /*lpFilename = */ path.data(),
      /*nSize      = */ static_cast<DWORD>(MAX_PATH))};
  if (kLength == 0 ||
      kLength + kSuffixChars > static_cast<std::size_t>(MAX_PATH)) {
    return;
  }
  std::wmemcpy(path.data() + kLength, L".log", kSuffixChars);

  const afw::win::ScopedHandle kFile{CreateFileW(
      /*lpFileName            = */ path.data(),
      /*dwDesiredAccess       = */ FILE_APPEND_DATA,
      /*dwShareMode           = */ FILE_SHARE_READ | FILE_SHARE_WRITE,
      /*lpSecurityAttributes  = */ nullptr,
      /*dwCreationDisposition = */ OPEN_ALWAYS,
      /*dwFlagsAndAttributes  = */ FILE_ATTRIBUTE_NORMAL,
      /*hTemplateFile         = */ nullptr)};
  if (!kFile) {
    return;
  }

  DWORD written{};
  WriteFile(
      /*hFile                  = */ kFile.Get(),
      /*lpBuffer               = */ text,
      /*nNumberOfBytesToWrite  = */ static_cast<DWORD>(std::strlen(text)),
      /*lpNumberOfBytesWritten = */ &written,
      /*lpOverlapped           = */ nullptr);
}

void LogFailure(const char* step, const std::error_code& error) {
  std::array<char, kMessageChars> message{};
  std::snprintf(message.data(), message.size(), "[aimbot] %s failed: %s\n",
                step, error.message().c_str());
  Log(message.data());
}

void LogTable(const afw::hook::Vtable& table) {
  std::array<char, kMessageChars> message{};
  std::snprintf(message.data(), message.size(),
                "[aimbot] 2/7 vtable at %p, Present at %p\n",
                static_cast<const void*>(table.Get()),
                table.At(afw::render::kPresentSlot));
  Log(message.data());
}

void LogStatus(std::uint64_t frames) {
  const afw::games::fc3::Diagnostics kReport{
      afw::games::fc3::ReadDiagnostics()};
  const std::expected<afw::games::fc3::CameraBasis, std::error_code> kCamera{
      afw::games::fc3::ReadCamera()};

  std::array<char, kMessageChars> message{};
  if (!kCamera) {
    std::snprintf(message.data(), message.size(),
                  "[aimbot] frame %llu | view %llX | %s\n", frames,
                  static_cast<std::uint64_t>(kReport.cameraView),
                  kCamera.error().message().c_str());
  } else {
    std::snprintf(
        message.data(), message.size(),
        "[aimbot] frame %llu | view %llX | pos %.1f %.1f %.1f | fov %.4f | "
        "render %dx%d\n",
        frames, static_cast<std::uint64_t>(kReport.cameraView),
        static_cast<double>(kCamera->position.at(0)),
        static_cast<double>(kCamera->position.at(1)),
        static_cast<double>(kCamera->position.at(2)),
        static_cast<double>(kCamera->fovRadians), kReport.viewportWidth,
        kReport.viewportHeight);
  }

  Log(message.data());
}

void AFW_ENGINE_CALL RunAllGroupDetour(void* self, void* /*unused*/) {
  afw::games::fc3::TakeCensus(std::bit_cast<std::uintptr_t>(self));

  afw::games::fc3::RunAllGroupFn original{
      afw::games::fc3::OriginalRunAllGroup()};
  if (original != nullptr) {
    original(self, nullptr);
  }
}

HRESULT __stdcall PresentDetour(IDXGISwapChain* swapChain, UINT syncInterval,
                                UINT flags) {
  static std::uint64_t frames{0};
  ++frames;

  afw::games::fc3::PublishCensus();

  if (frames % kStatusInterval == 0) {
    LogStatus(frames);
  }

  const std::expected<void, std::error_code> kDrawn{
      afw::render::DrawOverlayFrame(swapChain, &afw::esp::DrawFrame)};
  if (!kDrawn) {
    static bool reported{false};
    if (!reported) {
      reported = true;
      LogFailure("drawing the overlay", kDrawn.error());
    }
  }

  afw::render::PresentFn original{afw::render::OriginalPresent()};
  if (original == nullptr) {
    return E_FAIL;
  }

  return original(swapChain, syncInterval, flags);
}

void Park() {
  Log("[aimbot] 7/7 running\n");
  Sleep(INFINITE);
}

// Nested rather than sequential: both hooks and both bindings are scope-bound,
// and they have to stay alive until the process ends.
void InstallUpdateHookAndPark() {
  std::expected<afw::hook::VmtHook, std::error_code> hook{
      afw::hook::VmtHook::Create(
          afw::hook::Vtable{afw::games::fc3::RunAllGroupSlot()}, 0)};
  if (!hook) {
    LogFailure("6/7 creating the update hook", hook.error());
    Park();
    return;
  }

  const std::expected<void, std::error_code> kSet{
      hook->SetDetour(&RunAllGroupDetour)};
  if (!kSet) {
    LogFailure("6/7 setting the update detour", kSet.error());
    Park();
    return;
  }

  const afw::games::fc3::UpdateBinding kBound{*hook};

  const std::expected<void, std::error_code> kEnabled{hook->Enable()};
  if (!kEnabled) {
    LogFailure("6/7 enabling the update hook", kEnabled.error());
    Park();
    return;
  }
  Log("[aimbot] 6/7 update hooked\n");

  Park();
}

DWORD WINAPI OnAttach(LPVOID /*parameter*/) {
  Log("[aimbot] 1/7 attached\n");

  const std::expected<afw::hook::Vtable, std::error_code> kTable{
      afw::render::ProbeSwapChainVtable()};
  if (!kTable) {
    LogFailure("2/7 probing the swap chain vtable", kTable.error());
    return 1;
  }
  LogTable(*kTable);

  std::expected<afw::hook::VmtHook, std::error_code> hook{
      afw::hook::VmtHook::Create(*kTable, afw::render::kPresentSlot)};
  if (!hook) {
    LogFailure("3/7 creating the Present hook", hook.error());
    return 1;
  }
  Log("[aimbot] 3/7 hook created\n");

  const std::expected<void, std::error_code> kSet{
      hook->SetDetour(&PresentDetour)};
  if (!kSet) {
    LogFailure("4/7 setting the Present detour", kSet.error());
    return 1;
  }
  Log("[aimbot] 4/7 detour set\n");

  const afw::render::PresentBinding kBound{*hook};

  const std::expected<void, std::error_code> kEnabled{hook->Enable()};
  if (!kEnabled) {
    LogFailure("5/7 enabling the Present hook", kEnabled.error());
    return 1;
  }
  Log("[aimbot] 5/7 Present hooked\n");

  InstallUpdateHookAndPark();

  return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID /*reserved*/) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(
        /*hLibModule = */ instance);
    const afw::win::ScopedHandle kThread{CreateThread(
        /*lpThreadAttributes = */ nullptr,
        /*dwStackSize        = */ 0,
        /*lpStartAddress     = */ OnAttach,
        /*lpParameter        = */ nullptr,
        /*dwCreationFlags    = */ 0,
        /*lpThreadId         = */ nullptr)};
  }
  return TRUE;
}
