#include <afw/core/error.hpp>
#include <afw/process/error.hpp>
#include <string>
#include <system_error>

namespace afw::process {

const char* CategoryName(Error /*tag*/) { return "afw::process"; }

std::string Describe(Error error) {
  switch (error) {
    case Error::ModuleNotFound:
      return "the module is not loaded in this process";
    case Error::NotReadable:
      return "the address is not readable: it is not committed, the page "
             "denies access, or the read would leave the region";
    default:
      return "unknown afw::process error";
  }
}

const std::error_category& ErrorCategory() { return core::CategoryOf<Error>(); }

// NOLINTNEXTLINE(readability-identifier-naming)
std::error_code make_error_code(Error error) {
  return core::MakeErrorCode(error);
}

}  // namespace afw::process
