#pragma once

#include <afw/process/memory.hpp>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

using EnginePtr = std::uint32_t;

// Every pointer the engine stores is four bytes wide, whatever this build's
// uintptr_t is, so reading one is a read plus a widening. Inline and in the
// header because every translation unit that walks the engine needs it.
[[nodiscard]] inline std::expected<std::uintptr_t, std::error_code> ReadPointer(
    std::uintptr_t address) noexcept {
  return process::ReadAt<EnginePtr>(address).transform(
      [](EnginePtr value) { return static_cast<std::uintptr_t>(value); });
}

}  // namespace afw::games::fc3
