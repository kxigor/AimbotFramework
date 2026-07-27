#pragma once

#include <windows.h>

#include <system_error>

namespace afw::win {

[[nodiscard]] inline std::error_code LastError() {
  return {static_cast<int>(GetLastError()), std::system_category()};
}

}  // namespace afw::win
