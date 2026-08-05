#pragma once

#include <afw/math/angle.hpp>
#include <afw/math/vec3.hpp>
#include <algorithm>
#include <cmath>

namespace afw::math {

[[nodiscard]] inline ViewAngles AnglesOf(const Vec3& direction) noexcept {
  const Vec3 kUnit{direction.Normalised()};
  const float kGround{std::hypot(kUnit.x, kUnit.y)};
  if (kGround <= 0.0F) {
    return ViewAngles{.yaw = 0.0F,
                      .pitch = kUnit.z >= 0.0F ? kQuarterTurn : -kQuarterTurn};
  }

  return ViewAngles{.yaw = std::atan2(kUnit.y, kUnit.x),
                    .pitch = std::atan2(kUnit.z, kGround)};
}

[[nodiscard]] inline ViewAngles AnglesTo(const Vec3& from,
                                         const Vec3& target) noexcept {
  return AnglesOf(target - from);
}

[[nodiscard]] inline float AngleBetween(const Vec3& lhs,
                                        const Vec3& rhs) noexcept {
  const Vec3 kLeft{lhs.Normalised()};
  const Vec3 kRight{rhs.Normalised()};
  if (kLeft.Length() <= 0.0F || kRight.Length() <= 0.0F) {
    return kHalfTurn;
  }

  return std::acos(std::clamp(kLeft.Dot(kRight), -1.0F, 1.0F));
}

}  // namespace afw::math
