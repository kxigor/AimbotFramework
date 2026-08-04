#pragma once

#include <afw/games/fc3/fc3.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/vec3.hpp>
#include <cstdint>
#include <optional>

namespace afw::aimbot {

using math::Vec3;
// Both live a layer down: the game binding needs the same angles and the same
// wrapping, and it must not depend on this library to get them.
using math::ViewAngles;
using math::WrapAngle;

struct Target {
  std::uintptr_t entity{};
  Vec3 point{};
  float error{};
  float distance{};
};

// What is allowed to be considered. Anything past either of these is not a near
// miss worth correcting.
struct Limits {
  float maxDistance{};
  float maxError{};
};

[[nodiscard]] ViewAngles AnglesOf(const Vec3& direction) noexcept;

[[nodiscard]] ViewAngles AnglesTo(const Vec3& from,
                                  const Vec3& target) noexcept;

// How far `target` is from `from`, the shorter way round on each axis.
[[nodiscard]] ViewAngles Difference(const ViewAngles& from,
                                    const ViewAngles& target) noexcept;

// The part of a correction to apply this frame. A share of 1 snaps, which is
// both obvious on screen and unlike anything a hand does.
[[nodiscard]] ViewAngles Share(const ViewAngles& correction,
                               float share) noexcept;

[[nodiscard]] float AngleBetween(const Vec3& lhs, const Vec3& rhs) noexcept;

// The head when the skeleton gave one up, the middle of the box otherwise. The
// centre is a fallback rather than a choice: it is the only point there is for
// an entity whose bones could not be read.
[[nodiscard]] Vec3 AimPoint(const games::fc3::EntityInfo& entity) noexcept;

// Scored on how far the eye already points at it, so the winner is whoever the
// player is closest to aiming at. Distance decides only whether an entity is
// considered at all -- ranking by it would swap targets whenever somebody
// walked past in front.
//
// Which entities count is the caller's business: this asks `wanted` and holds
// no opinion of its own about who is hostile.
template <typename Predicate>
[[nodiscard]] std::optional<Target> ChooseTarget(
    const games::fc3::EntitySnapshot& snapshot, const Vec3& eye,
    const Vec3& forward, const Limits& limits, Predicate wanted) noexcept {
  std::optional<Target> best;

  for (std::uint32_t index{0}; index < snapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{snapshot.items.at(index)};
    if (!wanted(kEntity)) {
      continue;
    }

    const Vec3 kPoint{AimPoint(kEntity)};
    const Vec3 kOffset{kPoint - eye};
    const float kDistance{kOffset.Length()};
    if (kDistance > limits.maxDistance) {
      continue;
    }

    const float kError{AngleBetween(forward, kOffset)};
    if (kError > limits.maxError) {
      continue;
    }
    if (best && best->error <= kError) {
      continue;
    }

    best = Target{.entity = kEntity.address,
                  .point = kPoint,
                  .error = kError,
                  .distance = kDistance};
  }

  return best;
}

}  // namespace afw::aimbot
