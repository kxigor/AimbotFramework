#pragma once

#include <string>
#include <system_error>

namespace afw::core {

// One category instance per library, identified by address, so two codes carry
// the same meaning only when they came from the same library. Each library
// supplies CategoryName and Describe for its own enum next to that enum, and
// argument-dependent lookup finds them from here.
template <typename Enum>
class ErrorCategory final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return CategoryName(Enum{});
  }

  [[nodiscard]] std::string message(int condition) const override {
    return Describe(static_cast<Enum>(condition));
  }
};

template <typename Enum>
[[nodiscard]] const std::error_category& CategoryOf() {
  static const ErrorCategory<Enum> kCategory{};
  return kCategory;
}

template <typename Enum>
[[nodiscard]] std::error_code MakeErrorCode(Enum error) {
  return std::error_code{static_cast<int>(error), CategoryOf<Enum>()};
}

}  // namespace afw::core
