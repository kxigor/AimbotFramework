#include <afw/hook/error.hpp>
#include <string>
#include <system_error>

namespace afw::hook {

namespace {

class HookErrorCategory final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return "afw::hook";
  }

  [[nodiscard]] std::string message(int condition) const override {
    switch (static_cast<Error>(condition)) {
      case Error::HookAlreadyApplied:
        return "the hook is already applied: enabling it again would overwrite "
               "the saved original pointer with the detour and leave the slot "
               "impossible to restore";
      case Error::HookNotApplied:
        return "the hook is not applied: there is no saved original to put "
               "back into the table slot";
      default:
        return "unknown afw::hook error";
    }
  }
};

}  // namespace

const std::error_category& ErrorCategory() {
  static const HookErrorCategory kCategory{};
  return kCategory;
}

// NOLINTNEXTLINE(readability-identifier-naming)
std::error_code make_error_code(Error error) {
  return std::error_code{static_cast<int>(error), ErrorCategory()};
}

}  // namespace afw::hook
