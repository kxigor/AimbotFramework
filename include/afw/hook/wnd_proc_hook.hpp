#pragma once

#include <windows.h>

#include <expected>
#include <system_error>

namespace afw::hook {

// Hook over a window procedure, installed with SetWindowLongPtrW and undone on
// destruction. Shaped like VmtHook: the factory is the only way in, so no
// half-valid object exists, and the original is captured before the detour is
// ever named.
class WndProcHook {
  struct AdoptTag {};

 public:
  [[nodiscard]] static std::expected<WndProcHook, std::error_code> Create(
      HWND window) noexcept;

  WndProcHook(AdoptTag tag, HWND window, WNDPROC original) noexcept;

  WndProcHook(const WndProcHook&) = delete;
  WndProcHook& operator=(const WndProcHook&) = delete;

  WndProcHook(WndProcHook&&) = delete;
  WndProcHook& operator=(WndProcHook&&) = delete;

  ~WndProcHook();

  [[nodiscard]] std::expected<void, std::error_code> SetDetour(
      WNDPROC detour) noexcept;

  [[nodiscard]] WNDPROC Original() const noexcept;

  [[nodiscard]] std::expected<void, std::error_code> Enable() noexcept;
  [[nodiscard]] std::expected<void, std::error_code> Disable() noexcept;

 private:
  [[nodiscard]] std::expected<void, std::error_code> Swap(
      WNDPROC value) noexcept;

  bool isEnabled_{false};
  WNDPROC detour_{nullptr};
  HWND window_{nullptr};
  WNDPROC original_{nullptr};
};

}  // namespace afw::hook
