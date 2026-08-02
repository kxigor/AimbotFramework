#pragma once

namespace afw::config {

struct Settings {
  bool espEnabled{true};
  bool menuVisible{true};
  bool unloadRequested{false};
};

[[nodiscard]] Settings& Current() noexcept;

}  // namespace afw::config
