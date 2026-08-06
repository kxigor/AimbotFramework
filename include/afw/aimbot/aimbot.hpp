#pragma once

#include <afw/games/fc3/fc3.hpp>
#include <afw/math/ray.hpp>
#include <afw/math/scale.hpp>
#include <afw/math/vec3.hpp>
#include <cstdint>
#include <optional>

namespace afw::aimbot {

struct Ranking {
  constexpr Ranking(float priority, float reach) noexcept
      : priority{priority}, reach{reach} {}

  float priority;
  float reach;
};

struct Target {
  // NOLINTNEXTLINE(readability-function-size)
  Target(std::uintptr_t entity, const math::Vec3& point, float error,
         float distance, float priority, float reach) noexcept
      : entity{entity},
        point{point},
        error{error},
        distance{distance},
        priority{priority},
        reach{reach} {}

  std::uintptr_t entity;
  math::Vec3 point;
  float error;
  float distance;
  float priority;
  float reach;
};

using RankFn =
    std::optional<Ranking> (*)(const games::fc3::EntityInfo& entity,
                               const games::fc3::PawnInfo* self) noexcept;

struct Limits {
  constexpr Limits(float maxDistance, float maxError) noexcept
      : maxDistance{maxDistance}, maxError{maxError} {}

  [[nodiscard]] constexpr float NearError(float error) const noexcept {
    return math::Nearness(error, maxError);
  }

  [[nodiscard]] constexpr float NearRange(float distance) const noexcept {
    return math::Nearness(distance, maxDistance);
  }

  float maxDistance;
  float maxError;
};

struct Weights {
  constexpr Weights(float aim, float distance, float priority,
                    float reach) noexcept
      : aim{aim}, distance{distance}, priority{priority}, reach{reach} {}

  float aim;
  float distance;
  float priority;
  float reach;
};

struct Choice {
  constexpr Choice(const Limits& limits, const Weights& weights) noexcept
      : limits{limits}, weights{weights} {}

  Limits limits;
  Weights weights;
};

[[nodiscard]] math::Vec3 AimPoint(
    const games::fc3::EntityInfo& entity) noexcept;

[[nodiscard]] std::optional<Target> CandidateOf(
    const games::fc3::EntityInfo& entity, const math::Ray& view,
    const Limits& limits, RankFn rank,
    const games::fc3::PawnInfo* self) noexcept;

[[nodiscard]] inline float ScoreOf(const Choice& choice,
                                   const Target& target) noexcept {
  const Weights& kWeights{choice.weights};
  const Limits& kLimits{choice.limits};

  // clang-format off
  return (kWeights.aim      * kLimits.NearError(target.error))
       + (kWeights.distance * kLimits.NearRange(target.distance))
       + (kWeights.priority * target.priority)
       + (kWeights.reach    * target.reach);
  // clang-format on
}

[[nodiscard]] std::optional<Target> ChooseTarget(
    const games::fc3::EntitySnapshot& snapshot, const math::Ray& view,
    const Choice& choice, RankFn rank,
    const games::fc3::PawnInfo* self) noexcept;

}  // namespace afw::aimbot
