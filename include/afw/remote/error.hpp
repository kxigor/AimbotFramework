#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace afw::remote {

enum class Error : std::uint8_t {
  LoadLibraryFailed = 1,
  RemoteThreadTimeout = 2,
};

[[nodiscard]] const std::error_category& ErrorCategory();

// NOLINTNEXTLINE(readability-identifier-naming)
[[nodiscard]] std::error_code make_error_code(Error error);

}  // namespace afw::remote

template <>
struct std::is_error_code_enum<afw::remote::Error> : std::true_type {};
