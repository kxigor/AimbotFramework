#include <windows.h>

#include <afw/hook/error.hpp>
#include <afw/hook/wnd_proc_hook.hpp>
#include <afw/win/error.hpp>
#include <expected>
#include <system_error>
#include <tuple>

namespace afw::hook {

namespace {

[[nodiscard]] WNDPROC AsWndProc(LONG_PTR value) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
  return reinterpret_cast<WNDPROC>(value);
}

[[nodiscard]] LONG_PTR AsLong(WNDPROC value) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<LONG_PTR>(value);
}

}  // namespace

std::expected<WndProcHook, std::error_code> WndProcHook::Create(
    HWND window) noexcept {
  if (window == nullptr || IsWindow(/*hWnd = */ window) == FALSE) {
    return std::unexpected{make_error_code(Error::WindowNotFound)};
  }

  // Zero is a legal procedure address, so the failure is told apart from the
  // value by clearing the thread's last error first.
  SetLastError(/*dwErrCode = */ 0);
  const LONG_PTR kCurrent{GetWindowLongPtrW(
      /*hWnd   = */ window,
      /*nIndex = */ GWLP_WNDPROC)};
  if (kCurrent == 0 && GetLastError() != 0) {
    return std::unexpected{win::LastError()};
  }

  return std::expected<WndProcHook, std::error_code>{
      std::in_place, AdoptTag{}, window, AsWndProc(kCurrent)};
}

WndProcHook::WndProcHook(AdoptTag /*tag*/, HWND window,
                         WNDPROC original) noexcept
    : window_{window}, original_{original} {}

WndProcHook::~WndProcHook() { std::ignore = Disable(); }

std::expected<void, std::error_code> WndProcHook::SetDetour(
    WNDPROC detour) noexcept {
  if (isEnabled_) {
    return std::unexpected{make_error_code(Error::HookAlreadyApplied)};
  }

  detour_ = detour;

  return {};
}

WNDPROC WndProcHook::Original() const noexcept { return original_; }

std::expected<void, std::error_code> WndProcHook::Enable() noexcept {
  if (isEnabled_) {
    return std::unexpected{make_error_code(Error::HookAlreadyApplied)};
  }
  if (detour_ == nullptr) {
    return std::unexpected{make_error_code(Error::DetourNotSet)};
  }

  const std::expected<void, std::error_code> kSwapped{Swap(detour_)};
  if (!kSwapped) {
    return kSwapped;
  }

  isEnabled_ = true;

  return {};
}

std::expected<void, std::error_code> WndProcHook::Disable() noexcept {
  if (!isEnabled_) {
    return std::unexpected{make_error_code(Error::HookNotApplied)};
  }

  const std::expected<void, std::error_code> kSwapped{Swap(original_)};
  if (!kSwapped) {
    return kSwapped;
  }

  isEnabled_ = false;

  return {};
}

std::expected<void, std::error_code> WndProcHook::Swap(WNDPROC value) noexcept {
  SetLastError(/*dwErrCode = */ 0);
  const LONG_PTR kPrevious{SetWindowLongPtrW(
      /*hWnd      = */ window_,
      /*nIndex    = */ GWLP_WNDPROC,
      /*dwNewLong = */ AsLong(value))};
  if (kPrevious == 0 && GetLastError() != 0) {
    return std::unexpected{win::LastError()};
  }

  return {};
}

}  // namespace afw::hook
