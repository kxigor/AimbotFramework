#pragma once

#include <windows.h>

#include <cstdint>
#include <expected>
#include <string_view>
#include <system_error>

namespace afw::process {

// A byte signature with holes: "50 8D 4E 78 E8 ?? ?? ?? ??". Two hex digits
// match a byte, ? or ?? matches anything. Whitespace between tokens is free.
//
// This is what an address survives a different build of the game with, where a
// number lifted out of a decompiler does not. The scan covers the module's own
// image and nothing else.
[[nodiscard]] std::expected<std::uintptr_t, std::error_code> FindPattern(
    PCWSTR module, std::string_view pattern) noexcept;

}  // namespace afw::process
