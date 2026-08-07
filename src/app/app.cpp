#include <windows.h>

#include <afw/app/app.hpp>
#include <afw/app/detour_guard.hpp>
#include <afw/app/fault.hpp>
#include <afw/app/game.hpp>
#include <afw/app/log.hpp>
#include <afw/app/module.hpp>
#include <afw/app/scoped_hooks.hpp>
#include <afw/config/config.hpp>
#include <afw/hook/hook.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/hook/vtable.hpp>
#include <afw/hook/wnd_proc_hook.hpp>
#include <afw/menu/menu.hpp>
#include <afw/render/dxgi/present_hook.hpp>
#include <afw/render/dxgi/probe.hpp>
#include <afw/render/overlay.hpp>
#include <afw/win/scoped_handle.hpp>
#include <array>
#include <atomic>
#include <cstddef>
#include <expected>
#include <system_error>

namespace afw::app {

namespace {

constexpr DWORD kWindowPollMilliseconds{50};
constexpr int kWindowPollTicks{200};

[[nodiscard]] std::atomic<Game*>& CurrentGame() noexcept {
  static std::atomic<Game*> game{nullptr};
  return game;
}

[[nodiscard]] HANDLE UnloadEvent() noexcept {
  static const win::ScopedHandle kEvent{CreateEventW(
      /*lpEventAttributes = */ nullptr,
      /*bManualReset      = */ TRUE,
      /*bInitialState     = */ FALSE,
      /*lpName            = */ nullptr)};

  return kEvent.Get();
}

[[nodiscard]] WNDPROC& OriginalWndProc() noexcept {
  static WNDPROC original{nullptr};
  return original;
}

class InputBinding {
 public:
  explicit InputBinding(const hook::Hook& hook) noexcept {
    OriginalWndProc() = hook.Original<WNDPROC>();
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

  if (menu::HandleMessage(window, message, wParam, lParam)) {
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
  menu::Draw();

  Game* game{CurrentGame().load()};
  if (game != nullptr) {
    game->OnFrame();
  }
}

HRESULT __stdcall PresentDetour(IDXGISwapChain* swapChain, UINT syncInterval,
                                UINT flags) {
  const DetourGuard kInside{};

  const std::expected<void, std::error_code> kDrawn{
      render::dxgi::DrawOverlayFrame(swapChain, &DrawOverlay)};
  if (!kDrawn) {
    static bool reported{false};
    if (!reported) {
      reported = true;
      LogFailure("drawing the overlay", kDrawn.error());
    }
  }

  if (config::Current().unloadRequested) {
    SetEvent(
        /*hEvent = */ UnloadEvent());
  }

  render::dxgi::PresentFn original{render::dxgi::OriginalPresent()};
  if (original == nullptr) {
    return E_FAIL;
  }

  return original(swapChain, syncInterval, flags);
}

void LogAttached(const Game& game) {
  Logf("[aimbot] attached to %s\n", game.Name());
}

void LogTable(const hook::Vtable& table) {
  Logf("[aimbot] vtable at %p, Present at %p\n",
       static_cast<const void*>(table.Get()),
       table.At(render::dxgi::kPresentSlot));
}

void RunUntilUnload() {
  Log("[aimbot] running\n");
  WaitForSingleObject(
      /*hHandle        = */ UnloadEvent(),
      /*dwMilliseconds = */ INFINITE);
  Log("[aimbot] unload requested\n");
}

[[nodiscard]] HWND WaitForOverlayWindow() noexcept {
  for (int tick{0}; tick < kWindowPollTicks; ++tick) {
    HWND window{render::OverlayWindow()};
    if (window != nullptr) {
      return window;
    }
    Sleep(
        /*dwMilliseconds = */ kWindowPollMilliseconds);
  }

  return nullptr;
}

void AddInputHook(ScopedHooks& hooks) {
  HWND window{WaitForOverlayWindow()};
  if (window == nullptr) {
    Log("[aimbot] no overlay window, input not hooked\n");
    return;
  }

  hooks.Add<InputBinding>(hook::WndProcHook::CreateDynamic(window),
                          &WndProcDetour, "input");
}

void RunSession(Game& game) {
  const std::expected<hook::Vtable, std::error_code> kTable{
      render::dxgi::ProbeSwapChainVtable()};
  if (!kTable) {
    LogFailure("probing the swap chain vtable", kTable.error());
    return;
  }
  LogTable(*kTable);

  ScopedHooks hooks;
  if (!hooks.Add<render::dxgi::PresentBinding>(
          hook::VmtHook::CreateDynamic(*kTable, render::dxgi::kPresentSlot),
          &PresentDetour, "Present")) {
    return;
  }

  game.InstallHooks(hooks);
  AddInputHook(hooks);

  RunUntilUnload();
}

}  // namespace

void Run(Game& game) noexcept {
  LogAttached(game);
  CurrentGame().store(&game);

  {
    const FaultLogger kFaults{};
    RunSession(game);
  }

  WaitForDetours();
  CurrentGame().store(nullptr);
  render::ShutdownOverlay();
  Log("[aimbot] unloaded\n");

  const HMODULE kSelf{SelfModule()};
  if (kSelf != nullptr) {
    FreeLibraryAndExitThread(
        /*hLibModule = */ kSelf,
        /*dwExitCode = */ 0);
  }

  ExitThread(
      /*dwExitCode = */ 0);
}

}  // namespace afw::app
