#pragma once

#include <windows.h>

#include <afw/hook/hook.hpp>
#include <expected>
#include <memory>
#include <system_error>

namespace afw::hook {

class WndProcHook final : public Hook {
  struct AdoptTag {};

 public:
  [[nodiscard]] static std::expected<WndProcHook, std::error_code> Create(
      HWND window) noexcept;

  [[nodiscard]] static std::expected<std::unique_ptr<WndProcHook>,
                                     std::error_code>
  CreateDynamic(HWND window) noexcept;

  WndProcHook(AdoptTag tag, HWND window, void* original) noexcept;

  WndProcHook(const WndProcHook&) = delete;
  WndProcHook& operator=(const WndProcHook&) = delete;

  WndProcHook(WndProcHook&&) = delete;
  WndProcHook& operator=(WndProcHook&&) = delete;

  ~WndProcHook() override;

  [[nodiscard]] std::expected<void, std::error_code> Enable() noexcept override;
  [[nodiscard]] std::expected<void, std::error_code> Disable() noexcept
      override;

 private:
  [[nodiscard]] std::expected<void, std::error_code> SetDetourRaw(
      void* detour) noexcept override;

  [[nodiscard]] void* OriginalRaw() const noexcept override;

  [[nodiscard]] std::expected<void, std::error_code> Swap(void* value) noexcept;

  bool isEnabled_{false};
  void* detour_{nullptr};
  HWND window_{nullptr};
  void* original_{nullptr};
};

}  // namespace afw::hook
