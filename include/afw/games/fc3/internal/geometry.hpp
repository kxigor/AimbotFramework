#pragma once

#include <afw/games/fc3/internal/offsets.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <cmath>

namespace afw::games::fc3 {

// Checks on what was read rather than geometry for its own sake: a wrong
// pointer chain yields floats that fail these by orders. The vector algebra
// itself lives on Vec3; what is here is the part that carries Far Cry 3's own
// numbers.

// The update lists also hold components, whose +0x50 is not a position. Only an
// entity has a rigid transform at +0x20, so this both identifies one and
// validates the numbers the box is built from.
[[nodiscard]] inline bool IsOrthonormal(const Vec3& axisX, const Vec3& axisY,
                                        const Vec3& axisZ) noexcept {
  if (!axisX.IsUnitLength(kUnitTolerance) ||
      !axisY.IsUnitLength(kUnitTolerance) ||
      !axisZ.IsUnitLength(kUnitTolerance)) {
    return false;
  }

  return std::fabs(axisX.Dot(axisY)) < kAxisTolerance &&
         std::fabs(axisX.Dot(axisZ)) < kAxisTolerance &&
         std::fabs(axisY.Dot(axisZ)) < kAxisTolerance;
}

[[nodiscard]] inline bool IsSaneSpan(float low, float high) noexcept {
  const float kSpan{high - low};

  return std::isfinite(kSpan) && kSpan >= 0.0F && kSpan <= kMaxBoxSpan;
}

[[nodiscard]] inline bool IsSaneBox(const Vec3& low,
                                    const Vec3& high) noexcept {
  return IsSaneSpan(low.x, high.x) && IsSaneSpan(low.y, high.y) &&
         IsSaneSpan(low.z, high.z);
}

[[nodiscard]] inline bool IsInTheWorld(const Vec3& position) noexcept {
  if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
      !std::isfinite(position.z)) {
    return false;
  }
  if (std::fabs(position.x) > kMaxWorldCoordinate ||
      std::fabs(position.y) > kMaxWorldCoordinate ||
      std::fabs(position.z) > kMaxWorldCoordinate) {
    return false;
  }

  return std::fabs(position.x - kTemplateCoordinate) > kTemplateTolerance ||
         std::fabs(position.y - kTemplateCoordinate) > kTemplateTolerance;
}

// A humanoid's object-space box is min (-0.6,-0.6,0), max (+0.6,+0.6,+1.70).
[[nodiscard]] inline bool IsHumanoidBox(const Vec3& low,
                                        const Vec3& high) noexcept {
  return high.z > kHumanoidMinHeight && high.z < kHumanoidMaxHeight &&
         std::fabs(low.z) < kHumanoidFootTolerance &&
         std::fabs(high.x) < kHumanoidMaxWidth &&
         std::fabs(high.y) < kHumanoidMaxWidth;
}

}  // namespace afw::games::fc3
