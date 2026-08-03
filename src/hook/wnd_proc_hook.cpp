#include <windows.h>

#include <afw/hook/error.hpp>
#include <afw/hook/wnd_proc_hook.hpp>
#include <afw/win/error.hpp>
#include <bit>
#include <expected>
#include <memory>
#include <new>
#include <system_error>
#include <tuple>

namespace afw::hook {

namespace {

[[nodiscard]] std::expected<void*, std::error_code> Resolve(
    HWND window) noexcept {
  if (window == nullptr || IsWindow(/*hWnd = */ window) == FALSE) {
    return std::unexpected{make_error_code(Error::WindowNotFound)};
  }

  SetLastError(/*dwErrCode = */ 0);
  const LONG_PTR kCurrent{GetWindowLongPtrW(
      /*hWnd   = */ window,
      /*nIndex = */ GWLP_WNDPROC)};
  if (kCurrent == 0 && GetLastError() != 0) {
    return std::unexpected{win::LastError()};
  }

  return std::bit_cast<void*>(kCurrent);
}

}  // namespace

std::expected<WndProcHook, std::error_code> WndProcHook::Create(
    HWND window) noexcept {
  const std::expected<void*, std::error_code> kOriginal{Resolve(window)};
  if (!kOriginal) {
    return std::unexpected{kOriginal.error()};
  }

  return std::expected<WndProcHook, std::error_code>{std::in_place, AdoptTag{},
                                                     window, *kOriginal};
}

std::expected<std::unique_ptr<WndProcHook>, std::error_code>
WndProcHook::CreateDynamic(HWND window) noexcept {
  const std::expected<void*, std::error_code> kOriginal{Resolve(window)};
  if (!kOriginal) {
    return std::unexpected{kOriginal.error()};
  }

  std::unique_ptr<WndProcHook> hook{
      new (std::nothrow) WndProcHook{AdoptTag{}, window, *kOriginal}};
  if (!hook) {
    return std::unexpected{std::make_error_code(std::errc::not_enough_memory)};
  }

  return hook;
}

WndProcHook::WndProcHook(AdoptTag /*tag*/, HWND window, void* original) noexcept
    : window_{window}, original_{original} {}

WndProcHook::~WndProcHook() { std::ignore = Disable(); }

std::expected<void, std::error_code> WndProcHook::SetDetourRaw(
    void* detour) noexcept {
  if (isEnabled_) {
    return std::unexpected{make_error_code(Error::HookAlreadyApplied)};
  }

  detour_ = detour;

  return {};
}

void* WndProcHook::OriginalRaw() const noexcept { return original_; }

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

std::expected<void, std::error_code> WndProcHook::Swap(void* value) noexcept {
  SetLastError(/*dwErrCode = */ 0);
  const LONG_PTR kPrevious{SetWindowLongPtrW(
      /*hWnd      = */ window_,
      /*nIndex    = */ GWLP_WNDPROC,
      /*dwNewLong = */ std::bit_cast<LONG_PTR>(value))};
  if (kPrevious == 0 && GetLastError() != 0) {
    return std::unexpected{win::LastError()};
  }

  return {};
}

}  // namespace afw::hook
