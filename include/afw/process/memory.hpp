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

// Drops the per-thread cache of readable regions. Call it on a frame
// boundary so a region the game released cannot stay trusted for long.
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
  std::memcpy(&value, source, sizeof(T));

  return value;
}

}  // namespace afw::process
