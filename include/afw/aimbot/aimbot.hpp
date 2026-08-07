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
  Target(std::uintptr_t entity, const math::Vec3& point, float error,
         float distance, const Ranking& ranking) noexcept
      : entity{entity},
        point{point},
        error{error},
        distance{distance},
        ranking{ranking} {}

  std::uintptr_t entity;
  math::Vec3 point;
  float error;
  float distance;
  Ranking ranking;
};

using RankFn =
    std::optional<Ranking> (*)(const games::fc3::EntityInfo& entity,
                               const games::fc3::PawnInfo* self) noexcept;

class Limits {
 public:
  constexpr Limits(float maxDistance, float maxError) noexcept
      : maxDistance_{maxDistance}, maxError_{maxError} {}

  [[nodiscard]] constexpr bool InRange(float distance) const noexcept {
    return distance <= maxDistance_;
  }

  [[nodiscard]] constexpr bool InCone(float error) const noexcept {
    return error <= maxError_;
  }

  [[nodiscard]] constexpr float NearError(float error) const noexcept {
    return math::Nearness(error, maxError_);
  }

  [[nodiscard]] constexpr float NearRange(float distance) const noexcept {
    return math::Nearness(distance, maxDistance_);
  }

 private:
  float maxDistance_;
  float maxError_;
};

class Weights {
 public:
  constexpr Weights(float aim, float distance, float priority,
                    float reach) noexcept
      : aim_{aim}, distance_{distance}, priority_{priority}, reach_{reach} {}

  [[nodiscard]] constexpr bool WantsReach() const noexcept {
    return reach_ > 0.0F;
  }

  [[nodiscard]] constexpr float Applied(float nearError, float nearRange,
                                        const Ranking& ranking) const noexcept {
    // clang-format off
    return (aim_      * nearError)
         + (distance_ * nearRange)
         + (priority_ * ranking.priority)
         + (reach_    * ranking.reach);
    // clang-format on
  }

 private:
  float aim_;
  float distance_;
  float priority_;
  float reach_;
};

class Choice {
 public:
  constexpr Choice(const Limits& limits, const Weights& weights) noexcept
      : limits_{limits}, weights_{weights} {}

  [[nodiscard]] constexpr bool InRange(float distance) const noexcept {
    return limits_.InRange(distance);
  }

  [[nodiscard]] constexpr bool InCone(float error) const noexcept {
    return limits_.InCone(error);
  }

  [[nodiscard]] constexpr bool WantsReach() const noexcept {
    return weights_.WantsReach();
  }

  [[nodiscard]] constexpr float ScoreOf(const Target& target) const noexcept {
    return weights_.Applied(limits_.NearError(target.error),
                            limits_.NearRange(target.distance), target.ranking);
  }

 private:
  Limits limits_;
  Weights weights_;
};

[[nodiscard]] math::Vec3 AimPoint(
    const games::fc3::EntityInfo& entity) noexcept;

[[nodiscard]] std::optional<Target> CandidateOf(
    const games::fc3::EntityInfo& entity, const math::Ray& view,
    const Choice& choice, RankFn rank,
    const games::fc3::PawnInfo* self) noexcept;

[[nodiscard]] std::optional<Target> ChooseTarget(
    const games::fc3::EntitySnapshot& snapshot, const math::Ray& view,
    const Choice& choice, RankFn rank,
    const games::fc3::PawnInfo* self) noexcept;

}  // namespace afw::aimbot
