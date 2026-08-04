#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace afw::process {

enum class Error : std::uint8_t {
  ModuleNotFound = 1,
  NotReadable = 2,
  NotWritable = 3,
};

[[nodiscard]] const std::error_category& ErrorCategory();

// NOLINTNEXTLINE(readability-identifier-naming)
[[nodiscard]] std::error_code make_error_code(Error error);

}  // namespace afw::process

template <>
struct std::is_error_code_enum<afw::process::Error> : std::true_type {};
