#include <gtest/gtest.h>
#include <windows.h>

#include <afw/hook/error.hpp>
#include <afw/hook/wnd_proc_hook.hpp>
#include <bit>
#include <expected>
#include <memory>
#include <system_error>

namespace {

constexpr PCWSTR kClassName{L"AfwWndProcHookProbe"};

LRESULT __stdcall Detour(HWND window, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  return DefWindowProcW(window, message, wParam, lParam);
}

// A message-only window: never shown, never pumped, but a real HWND with a real
// procedure to swap.
class ProbeWindow {
 public:
  ProbeWindow() {
    WNDCLASSEXW description{};
    description.cbSize = sizeof(description);
    description.lpfnWndProc = &DefWindowProcW;
    description.hInstance = GetModuleHandleW(nullptr);
    description.lpszClassName = kClassName;
    atom_ = RegisterClassExW(&description);

    window_ = CreateWindowExW(0, kClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE,
                              nullptr, description.hInstance, nullptr);
  }

  ProbeWindow(const ProbeWindow&) = delete;
  ProbeWindow& operator=(const ProbeWindow&) = delete;

  ProbeWindow(ProbeWindow&&) = delete;
  ProbeWindow& operator=(ProbeWindow&&) = delete;

  ~ProbeWindow() {
    if (window_ != nullptr) {
      DestroyWindow(window_);
    }
    if (atom_ != 0) {
      UnregisterClassW(kClassName, GetModuleHandleW(nullptr));
    }
  }

  [[nodiscard]] HWND Get() const noexcept { return window_; }

 private:
  ATOM atom_{0};
  HWND window_{nullptr};
};

[[nodiscard]] WNDPROC CurrentProc(HWND window) noexcept {
  return std::bit_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
}

}  // namespace

TEST(WndProcHookUnit, RefusesAHandleThatNamesNoWindow) {
  const std::expected<afw::hook::WndProcHook, std::error_code> kHook{
      afw::hook::WndProcHook::Create(nullptr)};

  ASSERT_FALSE(kHook.has_value());
  EXPECT_EQ(kHook.error(),
            afw::hook::make_error_code(afw::hook::Error::WindowNotFound));
}

TEST(WndProcHookUnit, CapturesTheOriginalBeforeAnyDetourIsNamed) {
  const ProbeWindow kWindow{};
  ASSERT_NE(kWindow.Get(), nullptr);

  const std::expected<afw::hook::WndProcHook, std::error_code> kHook{
      afw::hook::WndProcHook::Create(kWindow.Get())};

  ASSERT_TRUE(kHook.has_value());
  EXPECT_EQ(kHook->Original<WNDPROC>(), CurrentProc(kWindow.Get()));
}

TEST(WndProcHookUnit, CreateDynamicCapturesTheSameOriginalOnTheHeap) {
  const ProbeWindow kWindow{};
  ASSERT_NE(kWindow.Get(), nullptr);

  const std::expected<std::unique_ptr<afw::hook::WndProcHook>, std::error_code>
      kHook{afw::hook::WndProcHook::CreateDynamic(kWindow.Get())};

  ASSERT_TRUE(kHook.has_value());
  ASSERT_NE(kHook->get(), nullptr);
  EXPECT_EQ((*kHook)->Original<WNDPROC>(), CurrentProc(kWindow.Get()));
}

TEST(WndProcHookUnit, CreateDynamicRefusesANonWindowTheSameWay) {
  const std::expected<std::unique_ptr<afw::hook::WndProcHook>, std::error_code>
      kHook{afw::hook::WndProcHook::CreateDynamic(nullptr)};

  ASSERT_FALSE(kHook.has_value());
  EXPECT_EQ(kHook.error(),
            afw::hook::make_error_code(afw::hook::Error::WindowNotFound));
}

TEST(WndProcHookUnit, RefusesToEnableWithoutADetour) {
  const ProbeWindow kWindow{};
  ASSERT_NE(kWindow.Get(), nullptr);

  std::expected<afw::hook::WndProcHook, std::error_code> hook{
      afw::hook::WndProcHook::Create(kWindow.Get())};
  ASSERT_TRUE(hook.has_value());

  const std::expected<void, std::error_code> kEnabled{hook->Enable()};

  ASSERT_FALSE(kEnabled.has_value());
  EXPECT_EQ(kEnabled.error(),
            afw::hook::make_error_code(afw::hook::Error::DetourNotSet));
}

TEST(WndProcHookUnit, SwapsTheProcedureAndPutsItBack) {
  const ProbeWindow kWindow{};
  ASSERT_NE(kWindow.Get(), nullptr);
  const WNDPROC kBefore{CurrentProc(kWindow.Get())};

  {
    std::expected<afw::hook::WndProcHook, std::error_code> hook{
        afw::hook::WndProcHook::Create(kWindow.Get())};
    ASSERT_TRUE(hook.has_value());
    ASSERT_TRUE(hook->SetDetour(&Detour).has_value());
    ASSERT_TRUE(hook->Enable().has_value());

    EXPECT_EQ(CurrentProc(kWindow.Get()), &Detour);
  }

  EXPECT_EQ(CurrentProc(kWindow.Get()), kBefore);
}

TEST(WndProcHookUnit, RejectsASecondDetourWhileEnabled) {
  const ProbeWindow kWindow{};
  ASSERT_NE(kWindow.Get(), nullptr);

  std::expected<afw::hook::WndProcHook, std::error_code> hook{
      afw::hook::WndProcHook::Create(kWindow.Get())};
  ASSERT_TRUE(hook.has_value());
  ASSERT_TRUE(hook->SetDetour(&Detour).has_value());
  ASSERT_TRUE(hook->Enable().has_value());

  const std::expected<void, std::error_code> kSecond{hook->SetDetour(&Detour)};

  ASSERT_FALSE(kSecond.has_value());
  EXPECT_EQ(kSecond.error(),
            afw::hook::make_error_code(afw::hook::Error::HookAlreadyApplied));
}
