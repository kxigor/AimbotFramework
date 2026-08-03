#pragma once

#include <bit>
#include <expected>
#include <system_error>
#include <type_traits>

namespace afw::hook {

template <typename Fn>
concept SlotFunction =
    std::is_pointer_v<Fn> && std::is_function_v<std::remove_pointer_t<Fn>> &&
    sizeof(Fn) == sizeof(void*);

class Hook {
 public:
  Hook(const Hook&) = delete;
  Hook& operator=(const Hook&) = delete;

  Hook(Hook&&) = delete;
  Hook& operator=(Hook&&) = delete;

  virtual ~Hook() = default;

  template <SlotFunction Fn>
  [[nodiscard]] std::expected<void, std::error_code> SetDetour(
      Fn detour) noexcept {
    return SetDetourRaw(std::bit_cast<void*>(detour));
  }

  template <SlotFunction Fn>
  [[nodiscard]] Fn Original() const noexcept {
    return std::bit_cast<Fn>(OriginalRaw());
  }

  [[nodiscard]] virtual std::expected<void, std::error_code>
  Enable() noexcept = 0;

  [[nodiscard]] virtual std::expected<void, std::error_code>
  Disable() noexcept = 0;

 protected:
  Hook() = default;

 private:
  [[nodiscard]] virtual std::expected<void, std::error_code> SetDetourRaw(
      void* detour) noexcept = 0;

  [[nodiscard]] virtual void* OriginalRaw() const noexcept = 0;
};

}  // namespace afw::hook
