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
        return "the hook is already applied: the table slot already holds the "
               "detour";
      case Error::HookNotApplied:
        return "the hook is not applied: there is no saved original to put "
               "back into the table slot";
      case Error::DetourNotSet:
        return "the hook has no detour: SetDetour must name one before the "
               "hook can be enabled";
      case Error::SlotNotReadable:
        return "the table slot was not readable when the hook was constructed: "
               "the address is not committed, or the page denies access";
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
