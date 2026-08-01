#include <afw/process/error.hpp>
#include <string>
#include <system_error>

namespace afw::process {

namespace {

class ProcessErrorCategory final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return "afw::process";
  }

  [[nodiscard]] std::string message(int condition) const override {
    switch (static_cast<Error>(condition)) {
      case Error::ModuleNotFound:
        return "the module is not loaded in this process";
      case Error::NotReadable:
        return "the address is not readable: it is not committed, the page "
               "denies access, or the read would leave the region";
      default:
        return "unknown afw::process error";
    }
  }
};

}  // namespace

const std::error_category& ErrorCategory() {
  static const ProcessErrorCategory kCategory{};
  return kCategory;
}

// NOLINTNEXTLINE(readability-identifier-naming)
std::error_code make_error_code(Error error) {
  return std::error_code{static_cast<int>(error), ErrorCategory()};
}

}  // namespace afw::process
