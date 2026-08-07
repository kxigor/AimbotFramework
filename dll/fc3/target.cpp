#include <afw/aimbot/aimbot.hpp>
#include <afw/config/config.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/kind.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/direction.hpp>
#include <afw/math/ray.hpp>
#include <afw/math/vec3.hpp>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

#include "internal.hpp"

namespace afw::dll {

namespace {

[[nodiscard]] std::uintptr_t& CameraStateCache() noexcept {
  static std::uintptr_t state{0};
  return state;
}

[[nodiscard]] std::optional<float> Ranked(bool wanted,
                                          float priority) noexcept {
  return wanted ? std::optional<float>{priority} : std::nullopt;
}

[[nodiscard]] std::optional<float> WantedKind(games::fc3::Kind kind) noexcept {
  using games::fc3::Kind;

  const config::Settings& kSettings{config::Current()};
  switch (kind) {
    case Kind::Enemy:
      return Ranked(kSettings.aimEnemies.load(),
                    kSettings.aimPriorityEnemies.load());
    case Kind::Predator:
    case Kind::Animal:
      return Ranked(kSettings.fc3.aimAnimals.load(),
                    kSettings.fc3.aimPriorityAnimals.load());
    case Kind::Bird:
      return Ranked(kSettings.fc3.aimBirds.load(),
                    kSettings.fc3.aimPriorityBirds.load());
    case Kind::Vehicle:
      return Ranked(kSettings.fc3.aimVehicles.load(),
                    kSettings.fc3.aimPriorityVehicles.load());
    case Kind::Self:
    case Kind::Ally:
    case Kind::Civilian:
    case Kind::Prop:
    default:
      return std::nullopt;
  }
}

[[nodiscard]] aimbot::Choice ChoiceFromSettings() noexcept {
  const config::Settings& kSettings{config::Current()};

  return aimbot::Choice{
      aimbot::Limits{
          /*maxDistance = */ kSettings.aimRange.load(),
          /*maxError    = */ math::HalfAngle(kSettings.aimConeDegrees.load())},
      aimbot::Weights{/*aim      = */ kSettings.aimWeightAim.load(),
                      /*distance = */ kSettings.aimWeightDistance.load(),
                      /*priority = */ kSettings.aimWeightPriority.load(),
                      /*reach    = */ kSettings.aimWeightReach.load()}};
}

[[nodiscard]] std::optional<aimbot::Ranking> RankAgainstSettings(
    const games::fc3::EntityInfo& entity,
    const games::fc3::PawnInfo* self) noexcept {
  const std::optional<float> kPriority{PriorityOf(entity, self)};
  if (!kPriority) {
    return std::nullopt;
  }

  return std::optional<aimbot::Ranking>{
      aimbot::Ranking{/*priority = */ *kPriority,
                      /*reach    = */ games::fc3::ReachOf(entity.address)}};
}

[[nodiscard]] math::Ray ViewOf(const games::fc3::CameraBasis& camera) noexcept {
  return math::Ray{.origin = camera.position, .direction = camera.forward};
}

}  // namespace

std::optional<float> PriorityOf(const games::fc3::EntityInfo& entity,
                                const games::fc3::PawnInfo* self) noexcept {
  if (!entity.pawn || entity.pawn->dead) {
    return std::nullopt;
  }

  return WantedKind(games::fc3::KindOf(entity, self));
}

std::optional<aimbot::Target> Pick(
    const games::fc3::EntitySnapshot& snapshot,
    const games::fc3::CameraBasis& camera) noexcept {
  return aimbot::ChooseTarget(snapshot, ViewOf(camera), ChoiceFromSettings(),
                              &RankAgainstSettings,
                              games::fc3::FindSelf(snapshot));
}

std::uintptr_t CameraState(const games::fc3::EntitySnapshot& snapshot,
                           const math::Vec3& forward) noexcept {
  if (CameraStateCache() != 0 &&
      games::fc3::ReproducesTheView(CameraStateCache(), forward)) {
    return CameraStateCache();
  }

  const games::fc3::EntityInfo* const kPlayer{games::fc3::SelfEntity(snapshot)};
  if (kPlayer == nullptr) {
    return 0;
  }
  CameraStateCache() =
      games::fc3::FindViewAngles(kPlayer->address, forward).value_or(0);

  return CameraStateCache();
}

void UpdateReach() noexcept {
  static std::uint64_t frames{0};
  ++frames;

  const aimbot::Choice kChoice{ChoiceFromSettings()};
  if (!kChoice.WantsReach()) {
    return;
  }

  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    return;
  }

  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{games::fc3::FindSelf(kSnapshot)};
  const math::Ray kView{ViewOf(*kCamera)};

  for (const games::fc3::EntityInfo& kEntity :
       games::fc3::Entities(kSnapshot)) {
    const std::optional<aimbot::Target> kCandidate{aimbot::CandidateOf(
        kEntity, kView, kChoice, &RankAgainstSettings, kSelf)};
    if (!kCandidate) {
      continue;
    }

    games::fc3::ObserveReach(kCandidate->entity, kCamera->position,
                             kCandidate->point, frames);
  }
}

}  // namespace afw::dll
