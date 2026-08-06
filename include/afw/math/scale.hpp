#pragma once

namespace afw::math {

[[nodiscard]] constexpr float Nearness(float value, float worst) noexcept {
  return worst > 0.0F ? 1.0F - (value / worst) : 0.0F;
}

}  // namespace afw::math
