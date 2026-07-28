#pragma once

// Hook over a single virtual method table slot: swaps one function pointer for
// a detour and puts the original back on destruction.

#include <windows.h>

#include <cstddef>
#include <expected>
#include <system_error>
#include <utility>

namespace afw::hook {
class VmtHook {
 public:
  VmtHook(void** vtable, std::size_t sourcePos, void* detour) noexcept;

  VmtHook(const VmtHook& /*unused*/) = delete;
  VmtHook(VmtHook&& /*unused*/) = delete;

  ~VmtHook();

  VmtHook& operator=(const VmtHook& /*unused*/) = delete;
  VmtHook& operator=(VmtHook&& /*unused*/) = delete;

  [[nodiscard]] std::expected<void, std::error_code> Enable() noexcept;
  [[nodiscard]] std::expected<void, std::error_code> Disable() noexcept;

 private:
  std::expected<void*, std::error_code> SwapSlot(void* value) noexcept;

  bool isEnabled_{false};
  void* original_{};

  void** vtable_{};

  const std::size_t kSourcePos_{};
  void* const kDetour_{};
};
}  // namespace afw::hook
