#include <afw/aimbot/aimbot.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/math/direction.hpp>
#include <afw/math/ray.hpp>
#include <afw/math/vec3.hpp>
#include <optional>

namespace afw::aimbot {

math::Vec3 AimPoint(const games::fc3::EntityInfo& entity) noexcept {
  if (entity.head) {
    return *entity.head;
  }
  if (entity.body) {
    return *entity.body;
  }

  return entity.transform.TransformPoint(entity.box.Centre());
}

std::optional<Target> CandidateOf(const games::fc3::EntityInfo& entity,
                                  const math::Ray& view, const Limits& limits,
                                  RankFn rank,
                                  const games::fc3::PawnInfo* self) noexcept {
  const std::optional<Ranking> kRanking{rank(entity, self)};
  if (!kRanking) {
    return std::nullopt;
  }

  const math::Vec3 kPoint{AimPoint(entity)};
  const math::Vec3 kOffset{kPoint - view.origin};
  const float kDistance{kOffset.Length()};
  if (kDistance > limits.maxDistance) {
    return std::nullopt;
  }

  const float kError{math::AngleBetween(view.direction, kOffset)};
  if (kError > limits.maxError) {
    return std::nullopt;
  }

  return Target{/*entity   = */ entity.address,
                /*point    = */ kPoint,
                /*error    = */ kError,
                /*distance = */ kDistance,
                /*priority = */ kRanking->priority,
                /*reach    = */ kRanking->reach};
}

std::optional<Target> ChooseTarget(const games::fc3::EntitySnapshot& snapshot,
                                   const math::Ray& view, const Choice& choice,
                                   RankFn rank,
                                   const games::fc3::PawnInfo* self) noexcept {
  std::optional<Target> best;
  float bestScore{};

  for (const games::fc3::EntityInfo& kEntity : games::fc3::Entities(snapshot)) {
    const std::optional<Target> kCandidate{
        CandidateOf(kEntity, view, choice.limits, rank, self)};
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
