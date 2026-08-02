#include <windows.h>

#include <afw/config/config.hpp>
#include <afw/core/cast.hpp>
#include <afw/esp/esp.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/hook/vtable.hpp>
#include <afw/hook/wnd_proc_hook.hpp>
#include <afw/menu/menu.hpp>
#include <afw/render/overlay.hpp>
#include <afw/render/present_hook.hpp>
#include <afw/render/render.hpp>
#include <afw/win/scoped_handle.hpp>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <expected>
#include <system_error>
#include <tuple>

namespace {

constexpr std::size_t kMessageChars{256};
constexpr std::size_t kSuffixChars{5};
constexpr std::uint64_t kStatusInterval{1800};
constexpr DWORD kDrainPollMilliseconds{10};
constexpr int kDrainTicks{200};
constexpr DWORD kSettleMilliseconds{100};
constexpr DWORD kWindowPollMilliseconds{50};
constexpr int kWindowPollTicks{200};

[[nodiscard]] HMODULE SelfModule() noexcept {
  HMODULE module{nullptr};
  if (GetModuleHandleExW(
          /*dwFlags      = */ GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          /*lpModuleName = */ afw::core::PointerCast<PCWSTR>(&SelfModule),
          /*phModule     = */ &module) == FALSE) {
    return nullptr;
  }

  return module;
}

void Log(const char* text) {
  OutputDebugStringA(text);

  const HMODULE kModule{SelfModule()};
  if (kModule == nullptr) {
    return;
  }

  std::array<WCHAR, MAX_PATH> path{};
  const DWORD kLength{GetModuleFileNameW(
      /*hModule    = */ kModule,
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
      afw::games::fc3::Diagnostics::Read()};
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
        static_cast<double>(kCamera->position.x),
        static_cast<double>(kCamera->position.y),
        static_cast<double>(kCamera->position.z),
        static_cast<double>(kCamera->fovRadians), kReport.viewportWidth,
        kReport.viewportHeight);
  }

  Log(message.data());
}

[[nodiscard]] std::atomic<int>& DetourDepth() noexcept {
  static std::atomic<int> depth{0};
  return depth;
}

class DetourGuard {
 public:
  DetourGuard() noexcept { DetourDepth().fetch_add(1); }

  DetourGuard(const DetourGuard&) = delete;
  DetourGuard& operator=(const DetourGuard&) = delete;

  DetourGuard(DetourGuard&&) = delete;
  DetourGuard& operator=(DetourGuard&&) = delete;

  ~DetourGuard() { DetourDepth().fetch_sub(1); }
};

[[nodiscard]] HANDLE UnloadEvent() noexcept {
  static const afw::win::ScopedHandle kEvent{CreateEventW(
      /*lpEventAttributes = */ nullptr,
      /*bManualReset      = */ TRUE,
      /*bInitialState     = */ FALSE,
      /*lpName            = */ nullptr)};

  return kEvent.Get();
}

WNDPROC& OriginalWndProc() noexcept {
  static WNDPROC original{nullptr};
  return original;
}

class InputBinding {
 public:
  explicit InputBinding(const afw::hook::WndProcHook& hook) noexcept {
    OriginalWndProc() = hook.Original();
  }

  InputBinding(const InputBinding&) = delete;
  InputBinding& operator=(const InputBinding&) = delete;

  InputBinding(InputBinding&&) = delete;
  InputBinding& operator=(InputBinding&&) = delete;

  ~InputBinding() { OriginalWndProc() = nullptr; }
};

LRESULT __stdcall WndProcDetour(HWND window, UINT message, WPARAM wParam,
                                LPARAM lParam) {
  const DetourGuard kInside{};

  if (afw::menu::HandleMessage(window, message, wParam, lParam)) {
    return TRUE;
  }

  WNDPROC original{OriginalWndProc()};
  if (original == nullptr) {
    return DefWindowProcW(window, message, wParam, lParam);
  }

  return CallWindowProcW(
      /*lpPrevWndFunc = */ original,
      /*hWnd          = */ window,
      /*Msg           = */ message,
      /*wParam        = */ wParam,
      /*lParam        = */ lParam);
}

void DrawOverlay() noexcept {
  afw::menu::Draw();
  afw::esp::DrawFrame();
}

void AFW_ENGINE_CALL RunAllGroupDetour(void* self, void* /*unused*/) {
  const DetourGuard kInside{};

  afw::games::fc3::TakeCensus(std::bit_cast<std::uintptr_t>(self));

  afw::games::fc3::RunAllGroupFn original{
      afw::games::fc3::OriginalRunAllGroup()};
  if (original != nullptr) {
    original(self, nullptr);
  }
}

HRESULT __stdcall PresentDetour(IDXGISwapChain* swapChain, UINT syncInterval,
                                UINT flags) {
  const DetourGuard kInside{};

  static std::uint64_t frames{0};
  ++frames;

  afw::games::fc3::PublishCensus();

  if (frames % kStatusInterval == 0) {
    LogStatus(frames);
  }

  const std::expected<void, std::error_code> kDrawn{
      afw::render::DrawOverlayFrame(swapChain, &DrawOverlay)};
  if (!kDrawn) {
    static bool reported{false};
    if (!reported) {
      reported = true;
      LogFailure("drawing the overlay", kDrawn.error());
    }
  }

  if (afw::config::Current().unloadRequested) {
    SetEvent(
        /*hEvent = */ UnloadEvent());
  }

  afw::render::PresentFn original{afw::render::OriginalPresent()};
  if (original == nullptr) {
    return E_FAIL;
  }

  return original(swapChain, syncInterval, flags);
}

void WaitForDetours() noexcept {
  for (int tick{0}; tick < kDrainTicks; ++tick) {
    if (DetourDepth().load() == 0) {
      break;
    }
    Sleep(
        /*dwMilliseconds = */ kDrainPollMilliseconds);
  }

  Sleep(
      /*dwMilliseconds = */ kSettleMilliseconds);
}

void RunUntilUnload() {
  Log("[aimbot] 8/8 running\n");
  WaitForSingleObject(
      /*hHandle        = */ UnloadEvent(),
      /*dwMilliseconds = */ INFINITE);
  Log("[aimbot] unload requested\n");
}

[[nodiscard]] HWND WaitForOverlayWindow() noexcept {
  for (int tick{0}; tick < kWindowPollTicks; ++tick) {
    HWND window{afw::render::OverlayWindow()};
    if (window != nullptr) {
      return window;
    }
    Sleep(
        /*dwMilliseconds = */ kWindowPollMilliseconds);
  }

  return nullptr;
}

void InstallInputHookAndRun() {
  HWND window{WaitForOverlayWindow()};
  if (window == nullptr) {
    Log("[aimbot] 7/8 no overlay window, input not hooked\n");
    RunUntilUnload();
    return;
  }

  std::expected<afw::hook::WndProcHook, std::error_code> hook{
      afw::hook::WndProcHook::Create(window)};
  if (!hook) {
    LogFailure("7/8 creating the input hook", hook.error());
    RunUntilUnload();
    return;
  }

  const std::expected<void, std::error_code> kSet{
      hook->SetDetour(&WndProcDetour)};
  if (!kSet) {
    LogFailure("7/8 setting the input detour", kSet.error());
    RunUntilUnload();
    return;
  }

  const InputBinding kBound{*hook};

  const std::expected<void, std::error_code> kEnabled{hook->Enable()};
  if (!kEnabled) {
    LogFailure("7/8 enabling the input hook", kEnabled.error());
    RunUntilUnload();
    return;
  }
  Log("[aimbot] 7/8 input hooked\n");

  RunUntilUnload();

  std::ignore = hook->Disable();
}

void InstallUpdateHookAndRun() {
  std::expected<afw::hook::VmtHook, std::error_code> hook{
      afw::hook::VmtHook::Create(
          afw::hook::Vtable{afw::games::fc3::RunAllGroupSlot()}, 0)};
  if (!hook) {
    LogFailure("6/8 creating the update hook", hook.error());
    InstallInputHookAndRun();
    return;
  }

  const std::expected<void, std::error_code> kSet{
      hook->SetDetour(&RunAllGroupDetour)};
  if (!kSet) {
    LogFailure("6/8 setting the update detour", kSet.error());
    InstallInputHookAndRun();
    return;
  }

  const afw::games::fc3::UpdateBinding kBound{*hook};

  const std::expected<void, std::error_code> kEnabled{hook->Enable()};
  if (!kEnabled) {
    LogFailure("6/8 enabling the update hook", kEnabled.error());
    InstallInputHookAndRun();
    return;
  }
  Log("[aimbot] 6/8 update hooked\n");

  InstallInputHookAndRun();

  std::ignore = hook->Disable();
}

void RunSession() {
  const std::expected<afw::hook::Vtable, std::error_code> kTable{
      afw::render::ProbeSwapChainVtable()};
  if (!kTable) {
    LogFailure("2/8 probing the swap chain vtable", kTable.error());
    return;
  }
  LogTable(*kTable);

  std::expected<afw::hook::VmtHook, std::error_code> hook{
      afw::hook::VmtHook::Create(*kTable, afw::render::kPresentSlot)};
  if (!hook) {
    LogFailure("3/8 creating the Present hook", hook.error());
    return;
  }
  Log("[aimbot] 3/8 hook created\n");

  const std::expected<void, std::error_code> kSet{
      hook->SetDetour(&PresentDetour)};
  if (!kSet) {
    LogFailure("4/8 setting the Present detour", kSet.error());
    return;
  }
  Log("[aimbot] 4/8 detour set\n");

  const afw::render::PresentBinding kBound{*hook};

  const std::expected<void, std::error_code> kEnabled{hook->Enable()};
  if (!kEnabled) {
    LogFailure("5/8 enabling the Present hook", kEnabled.error());
    return;
  }
  Log("[aimbot] 5/8 Present hooked\n");

  InstallUpdateHookAndRun();

  std::ignore = hook->Disable();
}

DWORD WINAPI OnAttach(LPVOID /*parameter*/) {
  Log("[aimbot] 1/8 attached\n");

  RunSession();

  WaitForDetours();
  afw::render::ShutdownOverlay();
  Log("[aimbot] unloaded\n");

  const HMODULE kSelf{SelfModule()};
  if (kSelf != nullptr) {
    FreeLibraryAndExitThread(
        /*hLibModule  = */ kSelf,
        /*dwExitCode  = */ 0);
  }

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
