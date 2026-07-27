#include <afw/remote/error.hpp>
#include <string>
#include <system_error>

namespace afw::remote {

namespace {

class RemoteErrorCategory final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return "afw::remote";
  }

  [[nodiscard]] std::string message(int condition) const override {
    switch (static_cast<Error>(condition)) {
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
};

}  // namespace

const std::error_category& ErrorCategory() {
  static const RemoteErrorCategory kCategory{};
  return kCategory;
}

// NOLINTNEXTLINE(readability-identifier-naming)
std::error_code make_error_code(Error error) {
  return std::error_code{static_cast<int>(error), ErrorCategory()};
}

}  // namespace afw::remote
