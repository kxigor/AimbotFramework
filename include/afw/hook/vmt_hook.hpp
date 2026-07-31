#pragma once

#include <afw/hook/vtable.hpp>
#include <bit>
#include <cstddef>
#include <expected>
#include <system_error>
#include <type_traits>

namespace afw::hook {

template <typename Fn>
concept SlotFunction =
    std::is_pointer_v<Fn> && std::is_function_v<std::remove_pointer_t<Fn>> &&
    sizeof(Fn) == sizeof(void*);

class VmtHook {
  struct AdoptTag {};

 public:
  [[nodiscard]] static std::expected<VmtHook, std::error_code> Create(
      Vtable table, std::size_t index) noexcept;

  VmtHook(AdoptTag tag, void** slot, void* original) noexcept;

  VmtHook(const VmtHook&) = delete;
  VmtHook& operator=(const VmtHook&) = delete;

  VmtHook(VmtHook&&) = delete;
  VmtHook& operator=(VmtHook&&) = delete;

  ~VmtHook();

  template <SlotFunction Fn>
  [[nodiscard]] std::expected<void, std::error_code> SetDetour(
      Fn detour) noexcept {
    return SetDetourRaw(std::bit_cast<void*>(detour));
  }

  template <SlotFunction Fn>
  [[nodiscard]] Fn Original() const noexcept {
    return std::bit_cast<Fn>(kOriginal_);
  }

  [[nodiscard]] std::expected<void, std::error_code> Enable() noexcept;
  [[nodiscard]] std::expected<void, std::error_code> Disable() noexcept;

 private:
  [[nodiscard]] std::expected<void, std::error_code> SetDetourRaw(
      void* detour) noexcept;

  [[nodiscard]] std::expected<void, std::error_code> SwapSlot(
      void* value) noexcept;

  bool isEnabled_{false};
  void* detour_{nullptr};

  void** const kSlot_{nullptr};
  void* const kOriginal_{nullptr};
};

}  // namespace afw::hook
