#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace afw::hook {

enum class Error : std::uint8_t {
  HookAlreadyApplied = 1,
  HookNotApplied = 2,
};

[[nodiscard]] const std::error_category& ErrorCategory();

// NOLINTNEXTLINE(readability-identifier-naming)
[[nodiscard]] std::error_code make_error_code(Error error);

}  // namespace afw::hook

template <>
struct std::is_error_code_enum<afw::hook::Error> : std::true_type {};
