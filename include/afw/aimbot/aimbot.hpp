#pragma once

#include <afw/games/fc3/fc3.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/vec3.hpp>
#include <cstdint>
#include <optional>

namespace afw::aimbot {

using math::Vec3;
using math::ViewAngles;
using math::WrapAngle;

struct Target {
  std::uintptr_t entity{};
  Vec3 point{};
  float error{};
  float distance{};
  float priority{};
  float reach{};
};

struct Ranking {
  float priority{};
  float reach{1.0F};
};

struct Limits {
  float maxDistance{};
  float maxError{};
};

[[nodiscard]] ViewAngles AnglesOf(const Vec3& direction) noexcept;

[[nodiscard]] ViewAngles AnglesTo(const Vec3& from,
                                  const Vec3& target) noexcept;

[[nodiscard]] ViewAngles Difference(const ViewAngles& from,
                                    const ViewAngles& target) noexcept;

[[nodiscard]] ViewAngles Share(const ViewAngles& correction,
                               float share) noexcept;

[[nodiscard]] float AngleBetween(const Vec3& lhs, const Vec3& rhs) noexcept;

[[nodiscard]] Vec3 AimPoint(const games::fc3::EntityInfo& entity) noexcept;

struct Weights {
  float aim{1.0F};
  float distance{};
  float priority{};
  float reach{};
};

struct Choice {
  Limits limits{};
  Weights weights{};
};

[[nodiscard]] constexpr float Nearness(float value, float worst) noexcept {
  return worst > 0.0F ? 1.0F - (value / worst) : 0.0F;
}

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
