#pragma once

#include <cstddef>
#include <expected>
#include <system_error>

namespace afw::hook {
class VmtHook {
 public:
  VmtHook(void** vtable, std::size_t sourcePos, void* detour) noexcept;

  VmtHook(const VmtHook&) = delete;
  VmtHook& operator=(const VmtHook&) = delete;

  VmtHook(VmtHook&&) = delete;
  VmtHook& operator=(VmtHook&&) = delete;

  ~VmtHook();

  [[nodiscard]] std::expected<void, std::error_code> Enable() noexcept;
  [[nodiscard]] std::expected<void, std::error_code> Disable() noexcept;

 private:
  [[nodiscard]] std::expected<void*, std::error_code> SwapSlot(
      void* value) noexcept;

  bool isEnabled_{false};
  void* original_{nullptr};

  void** vtable_{nullptr};

  const std::size_t kSourcePos_{};
  void* const kDetour_{nullptr};
};
}  // namespace afw::hook
