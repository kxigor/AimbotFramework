#include <imgui.h>
#include <imgui_impl_win32.h>
#include <windows.h>

#include <afw/config/config.hpp>
#include <afw/menu/menu.hpp>

// NOLINTNEXTLINE(readability-identifier-naming)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace afw::menu {

namespace {

constexpr WPARAM kToggleKey{VK_INSERT};
constexpr float kWidth{300.0F};
constexpr float kHeight{190.0F};

[[nodiscard]] bool IsInput(UINT message) noexcept {
  switch (message) {
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_SETCURSOR:
    case WM_INPUT:
      return true;
    default:
      return false;
  }
}

}  // namespace

void Draw() noexcept {
  config::Settings& settings{config::Current()};

  ImGui::GetIO().MouseDrawCursor = settings.menuVisible;
  if (!settings.menuVisible) {
    return;
  }

  ClipCursor(
      /*lpRect = */ nullptr);

  ImGui::SetNextWindowSize(ImVec2{kWidth, kHeight}, ImGuiCond_FirstUseEver);
  ImGui::Begin("AimbotFramework", &settings.menuVisible);

  ImGui::Text("%.1f FPS", static_cast<double>(ImGui::GetIO().Framerate));
  ImGui::SeparatorText("features");
  ImGui::Checkbox("ESP", &settings.espEnabled);

  ImGui::SeparatorText("session");
  ImGui::TextUnformatted("INSERT shows and hides this window");
  if (ImGui::Button("Unload")) {
    settings.unloadRequested = true;
  }

  ImGui::End();
}

bool HandleMessage(HWND window, UINT message, WPARAM wParam,
                   LPARAM lParam) noexcept {
  config::Settings& settings{config::Current()};

  if (message == WM_KEYDOWN && wParam == kToggleKey) {
    settings.menuVisible = !settings.menuVisible;
    return true;
  }

  static_cast<void>(
      ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam));

  return settings.menuVisible && IsInput(message);
}

}  // namespace afw::menu
