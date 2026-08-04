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
  // What the caller said this kind of target is worth to it.
  float priority{};
  // Whether a shot would get there at all.
  float reach{};
};

// What the caller thinks of one candidate. Reach is separate from priority
// because they answer different questions -- what a target is worth, and
// whether it can be hit -- and each carries its own weight.
struct Ranking {
  float priority{};
  float reach{1.0F};
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

// The head when the skeleton gave one up, then the spine, then the middle of
// the box. An animal is never given a head, so it is aimed at its spine: its
// box holds as much leg as body, and the middle of one falls under the belly of
// a boar rather than in it. The centre is a fallback rather than a choice: it
// is the only point there is for an entity whose bones could not be read.
[[nodiscard]] Vec3 AimPoint(const games::fc3::EntityInfo& entity) noexcept;

// Scored on how far the eye already points at it, so the winner is whoever the
// player is closest to aiming at. Distance and the caller's own ordering only
// count for as much as their weights say, which is nothing by default.
struct Weights {
  float aim{1.0F};
  float distance{};
  float priority{};
  float reach{};
};

// Everything the choice turns on: who may be considered at all, and how those
// who may are ranked against each other.
struct Choice {
  Limits limits{};
  Weights weights{};
};

// A term's share of the score, from 1 where `value` is nothing to 0 where it
// has reached `worst`. A limit of zero admits nobody, so nothing can score by
// it either.
[[nodiscard]] constexpr float Nearness(float value, float worst) noexcept {
  return worst > 0.0F ? 1.0F - (value / worst) : 0.0F;
}

// What one entity would be worth as a target, or nothing if it is not one:
// `rank` refuses it, it stands beyond the range, or it lies outside the cone.
template <typename Rank>
[[nodiscard]] std::optional<Target> Considered(
    const games::fc3::EntityInfo& entity, const Vec3& eye, const Vec3& forward,
    const Limits& limits, Rank rank) noexcept {
  const std::optional<Ranking> kRanking{rank(entity)};
  if (!kRanking) {
    return std::nullopt;
  }

  const Vec3 kPoint{AimPoint(entity)};
  const Vec3 kOffset{kPoint - eye};
  const float kDistance{kOffset.Length()};
  if (kDistance > limits.maxDistance) {
    return std::nullopt;
  }

  const float kError{AngleBetween(forward, kOffset)};
  if (kError > limits.maxError) {
    return std::nullopt;
  }

  return Target{.entity = entity.address,
                .point = kPoint,
                .error = kError,
                .distance = kDistance,
                .priority = kRanking->priority,
                .reach = kRanking->reach};
}

[[nodiscard]] inline float ScoreOf(const Choice& choice,
                                   const Target& target) noexcept {
  return (choice.weights.aim * Nearness(target.error, choice.limits.maxError)) +
         (choice.weights.distance *
          Nearness(target.distance, choice.limits.maxDistance)) +
         (choice.weights.priority * target.priority) +
         (choice.weights.reach * target.reach);
}

// Ranked, not merely filtered: every candidate inside the cone is scored and
// the best score wins. Each term is a 0..1 nearness before its weight applies,
// so the weights are directly comparable and any of them may be zero. All of
// them zero leaves every candidate tied on nothing, and the first one found
// wins by default.
//
// Which entities count is the caller's business: `rank` returns nothing for an
// entity that is not a target at all, and otherwise how much the caller wants
// that kind. This holds no opinion of its own about who is hostile.
template <typename Rank>
[[nodiscard]] std::optional<Target> ChooseTarget(
    const games::fc3::EntitySnapshot& snapshot, const Vec3& eye,
    const Vec3& forward, const Choice& choice, Rank rank) noexcept {
  std::optional<Target> best;
  float bestScore{};

  for (std::uint32_t index{0}; index < snapshot.count; ++index) {
    const std::optional<Target> kCandidate{Considered(
        snapshot.items.at(index), eye, forward, choice.limits, rank)};
    if (!kCandidate) {
      continue;
    }

    const float kScore{ScoreOf(choice, *kCandidate)};
    if (best && bestScore >= kScore) {
      continue;
    }

    bestScore = kScore;
    best = kCandidate;
  }

  return best;
}

}  // namespace afw::aimbot
