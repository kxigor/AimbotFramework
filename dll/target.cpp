#include <afw/aimbot/aimbot.hpp>
#include <afw/config/config.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/kind.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/direction.hpp>
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

[[nodiscard]] std::uintptr_t PlayerAddress(
    const games::fc3::EntitySnapshot& snapshot) noexcept {
  for (std::uint32_t index{0}; index < snapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{snapshot.items.at(index)};
    if (kEntity.pawn && kEntity.pawn->agentType == games::fc3::kAgentPlayer) {
      return kEntity.address;
    }
  }

  return 0;
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
      return Ranked(kSettings.aimAnimals.load(),
                    kSettings.aimPriorityAnimals.load());
    case Kind::Bird:
      return Ranked(kSettings.aimBirds.load(),
                    kSettings.aimPriorityBirds.load());
    case Kind::Vehicle:
      return Ranked(kSettings.aimVehicles.load(),
                    kSettings.aimPriorityVehicles.load());
    case Kind::Self:
    case Kind::Ally:
    case Kind::Civilian:
    case Kind::Prop:
    default:
      return std::nullopt;
  }
}

}  // namespace

float HalfConeRadians(float degrees) noexcept {
  return math::HalfAngle(degrees);
}

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
  const config::Settings& kSettings{config::Current()};
  const games::fc3::PawnInfo* const kSelf{games::fc3::FindSelf(snapshot)};
  const aimbot::Choice kChoice{
      .limits = {.maxDistance = kSettings.aimRange.load(),
                 .maxError = HalfConeRadians(kSettings.aimConeDegrees.load())},
      .weights = {.aim = kSettings.aimWeightAim.load(),
                  .distance = kSettings.aimWeightDistance.load(),
                  .priority = kSettings.aimWeightPriority.load(),
                  .reach = kSettings.aimWeightReach.load()}};

  return aimbot::ChooseTarget(
      snapshot, camera.position, camera.forward, kChoice,
      [kSelf](const games::fc3::EntityInfo& entity) {
        const std::optional<float> kPriority{PriorityOf(entity, kSelf)};
        if (!kPriority) {
          return std::optional<aimbot::Ranking>{};
        }

        return std::optional<aimbot::Ranking>{
            aimbot::Ranking{.priority = *kPriority,
                            .reach = games::fc3::ReachOf(entity.address)}};
      });
}

std::uintptr_t CameraState(const games::fc3::EntitySnapshot& snapshot,
                           const math::Vec3& forward) noexcept {
  if (CameraStateCache() != 0 &&
      games::fc3::ReproducesTheView(CameraStateCache(), forward)) {
    return CameraStateCache();
  }

  const std::uintptr_t kPlayer{PlayerAddress(snapshot)};
  if (kPlayer == 0) {
    return 0;
  }
  CameraStateCache() = games::fc3::FindViewAngles(kPlayer, forward).value_or(0);

  return CameraStateCache();
}

void UpdateReach() noexcept {
  static std::uint64_t frames{0};
  ++frames;

  if (config::Current().aimWeightReach.load() <= 0.0F) {
    return;
  }

  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    return;
  }

  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{games::fc3::FindSelf(kSnapshot)};
  const float kRange{config::Current().aimRange.load()};
  const float kCone{HalfConeRadians(config::Current().aimConeDegrees.load())};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    if (!PriorityOf(kEntity, kSelf)) {
      continue;
    }

    const math::Vec3 kPoint{aimbot::AimPoint(kEntity)};
    const math::Vec3 kOffset{kPoint - kCamera->position};
    if (kOffset.Length() > kRange ||
        math::AngleBetween(kCamera->forward, kOffset) > kCone) {
      continue;
    }

    games::fc3::ObserveReach(kEntity.address, kCamera->position, kPoint,
                             frames);
  }
}

}  // namespace afw::dll
