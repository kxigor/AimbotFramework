#pragma once

#include <atomic>

namespace afw::config {

struct Settings {
  std::atomic<bool> espEnabled{true};
  bool menuVisible{true};
  bool unloadRequested{false};
};

[[nodiscard]] Settings& Current() noexcept;

}  // namespace afw::config
