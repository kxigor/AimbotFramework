#pragma once

#include <windows.h>

#include <afw/process/error.hpp>
#include <bit>
#include <cstdint>
#include <cstring>
#include <expected>
#include <system_error>
#include <type_traits>

namespace afw::process {

[[nodiscard]] std::expected<std::uintptr_t, std::error_code> ModuleBase(
    PCWSTR name) noexcept;

[[nodiscard]] bool IsReadable(LPCVOID address, SIZE_T size) noexcept;

void ForgetReadableRegions() noexcept;

template <typename T>
requires std::is_trivially_copyable_v<T>
[[nodiscard]] std::expected<T, std::error_code> ReadAt(
    std::uintptr_t address) noexcept {
  LPCVOID source{std::bit_cast<LPCVOID>(address)};
  if (!IsReadable(source, sizeof(T))) {
    return std::unexpected{make_error_code(Error::NotReadable)};
  }

  T value{};
  std::memcpy(static_cast<void*>(&value), source, sizeof(T));

  return value;
}

template <typename TargetPointer>
requires std::is_unsigned_v<TargetPointer> &&
         (sizeof(TargetPointer) <= sizeof(std::uintptr_t))
[[nodiscard]] std::expected<std::uintptr_t, std::error_code> ReadPointerAs(
    std::uintptr_t address) noexcept {
  return ReadAt<TargetPointer>(address).transform(
      [](TargetPointer value) { return static_cast<std::uintptr_t>(value); });
}

}  // namespace afw::process
