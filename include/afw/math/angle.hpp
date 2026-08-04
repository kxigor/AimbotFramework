#pragma once

#include <cmath>
#include <numbers>

namespace afw::math {

constexpr float kHalfTurn{std::numbers::pi_v<float>};
constexpr float kFullTurn{2.0F * kHalfTurn};
constexpr float kQuarterTurn{kHalfTurn / 2.0F};

// Yaw turns around the world's up axis, pitch lifts away from the horizon.
// Both in radians. Which way round the game stores them, and where it measures
// yaw from, is the game binding's business, not this one's.
struct ViewAngles {
  float yaw{};
  float pitch{};
};

// Onto (-pi, pi], so the gap between two headings is always the shorter way
// round. Anything that is not a number wraps to zero instead of poisoning every
// comparison downstream of it.
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
