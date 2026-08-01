#include <afw/games/fc3/error.hpp>
#include <string>
#include <system_error>

namespace afw::games::fc3 {

namespace {

class Fc3ErrorCategory final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return "afw::games::fc3";
  }

  [[nodiscard]] std::string message(int condition) const override {
    switch (static_cast<Error>(condition)) {
      case Error::CameraUnavailable:
        return "no camera is active: the manager holds none, or the world is "
               "not loaded yet";
      case Error::CameraNotFound:
        return "the active camera stack entry holds nothing whose vtable "
               "matches the known camera class";
      case Error::CameraNotSane:
        return "the camera was found but its basis fails the sanity check: "
               "the forward vector is not unit length, or the field of view "
               "is out of range";
      case Error::ViewportNotSane:
        return "the viewport reports a width or height that is not positive";
      default:
        return "unknown afw::games::fc3 error";
    }
  }
};

}  // namespace

const std::error_category& ErrorCategory() {
  static const Fc3ErrorCategory kCategory{};
  return kCategory;
}

// NOLINTNEXTLINE(readability-identifier-naming)
std::error_code make_error_code(Error error) {
  return std::error_code{static_cast<int>(error), ErrorCategory()};
}

}  // namespace afw::games::fc3
