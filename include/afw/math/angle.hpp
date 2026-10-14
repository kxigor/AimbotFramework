#pragma once

#include <algorithm>
#include <cmath>
#include <compare>
#include <numbers>
#include <type_traits>

namespace afw::math {

constexpr float kHalfTurn{std::numbers::pi_v<float>};
constexpr float kFullTurn{2.0F * kHalfTurn};
constexpr float kQuarterTurn{kHalfTurn / 2.0F};
constexpr float kHalf{0.5F};
constexpr float kDegreesPerHalfTurn{180.0F};

struct Angle {
  [[nodiscard]] constexpr auto operator<=>(const Angle&) const = default;

  float radians{};
};

static_assert(std::is_trivially_copyable_v<Angle>);

struct ViewAngles {
  float yaw{};
  float pitch{};
};

[[nodiscard]] constexpr float Radians(float degrees) noexcept {
  return degrees * kHalfTurn / kDegreesPerHalfTurn;
}

[[nodiscard]] constexpr float Degrees(float radians) noexcept {
  return radians * kDegreesPerHalfTurn / kHalfTurn;
}

[[nodiscard]] constexpr float HalfAngle(float degrees) noexcept {
  return Radians(degrees * kHalf);
}

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

[[nodiscard]] inline ViewAngles Difference(const ViewAngles& from,
                                           const ViewAngles& target) noexcept {
  return ViewAngles{.yaw = WrapAngle(target.yaw - from.yaw),
                    .pitch = WrapAngle(target.pitch - from.pitch)};
}

[[nodiscard]] inline ViewAngles Share(const ViewAngles& correction,
                                      float share) noexcept {
  const float kClamped{std::clamp(share, 0.0F, 1.0F)};

  return ViewAngles{.yaw = correction.yaw * kClamped,
                    .pitch = correction.pitch * kClamped};
}

}  // namespace afw::math
