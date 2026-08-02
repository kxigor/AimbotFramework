#pragma once

namespace afw::config {

// Settings shared by the menu and the ESP. Neither owns the other's state, so
// the ESP runs off these alone with no user interface present.
struct Settings {
  bool espEnabled{true};
  bool menuVisible{true};
  bool unloadRequested{false};
};

[[nodiscard]] Settings& Current() noexcept;

}  // namespace afw::config
