#pragma once

#include <afw/hook/hook.hpp>
#include <afw/hook/vtable.hpp>
#include <cstddef>
#include <expected>
#include <memory>
#include <system_error>

namespace afw::hook {

class VmtHook final : public Hook {
  struct AdoptTag {};

 public:
  [[nodiscard]] static std::expected<VmtHook, std::error_code> Create(
      Vtable table, std::size_t index) noexcept;

  [[nodiscard]] static std::expected<std::unique_ptr<VmtHook>, std::error_code>
  CreateDynamic(Vtable table, std::size_t index) noexcept;

  VmtHook(AdoptTag tag, void** slot, void* original) noexcept;

  VmtHook(const VmtHook&) = delete;
  VmtHook& operator=(const VmtHook&) = delete;

  VmtHook(VmtHook&&) = delete;
  VmtHook& operator=(VmtHook&&) = delete;

  ~VmtHook() override;

  [[nodiscard]] std::expected<void, std::error_code> Enable() noexcept override;
  [[nodiscard]] std::expected<void, std::error_code> Disable() noexcept
      override;

 private:
  [[nodiscard]] std::expected<void, std::error_code> SetDetourRaw(
      void* detour) noexcept override;

  [[nodiscard]] void* OriginalRaw() const noexcept override;

  [[nodiscard]] std::expected<void, std::error_code> SwapSlot(
      void* value) noexcept;

  bool isEnabled_{false};
  void* detour_{nullptr};

  void** const kSlot_{nullptr};
  void* const kOriginal_{nullptr};
};

}  // namespace afw::hook
