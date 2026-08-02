#include <afw/config/config.hpp>

namespace afw::config {

Settings& Current() noexcept {
  static Settings settings{};
  return settings;
}

}  // namespace afw::config
