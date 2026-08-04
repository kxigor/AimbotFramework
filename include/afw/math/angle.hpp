#pragma once

#include <cmath>
#include <numbers>

namespace afw::math {

constexpr float kHalfTurn{std::numbers::pi_v<float>};
constexpr float kFullTurn{2.0F * kHalfTurn};
constexpr float kQuarterTurn{kHalfTurn / 2.0F};

struct ViewAngles {
  float yaw{};
  float pitch{};
};

[[nodiscard]] inline float WrapAngle(float radians) noexcept {
  if (!std::isfinite(radians)) {
    return 0.0F;
  }

  const float kWrapped{std::remainder(radians, kFullTurn)};
  if (kWrapped <= -kHalfTurn) {
    return kWrapped + kFullTurn;
  }

  return kWrapped;
}

}  // namespace afw::math
