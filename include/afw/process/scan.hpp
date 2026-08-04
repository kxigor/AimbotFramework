#pragma once

#include <windows.h>

#include <cstdint>
#include <expected>
#include <string_view>
#include <system_error>

namespace afw::process {

[[nodiscard]] std::expected<std::uintptr_t, std::error_code> FindPattern(
    PCWSTR module, std::string_view pattern) noexcept;

}  // namespace afw::process
