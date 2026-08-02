#include <afw/core/error.hpp>
#include <afw/remote/error.hpp>
#include <string>
#include <system_error>

namespace afw::remote {

const char* CategoryName(Error /*tag*/) { return "afw::remote"; }

std::string Describe(Error error) {
  switch (error) {
    case Error::LoadLibraryFailed:
      return "the remote LoadLibraryW returned NULL: the module was not "
             "found, its architecture does not match the target, or one of "
             "its dependencies failed to load";
    case Error::RemoteThreadTimeout:
      return "the remote thread did not finish within the timeout";
    default:
      return "unknown afw::remote error";
  }
}

const std::error_category& ErrorCategory() { return core::CategoryOf<Error>(); }

// NOLINTNEXTLINE(readability-identifier-naming)
std::error_code make_error_code(Error error) {
  return core::MakeErrorCode(error);
}

}  // namespace afw::remote
