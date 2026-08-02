#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/process/memory.hpp>
#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] bool IsTemplateSpawn(const Vec3& position) noexcept {
  return std::fabs(position.x - kTemplateCoordinate) <= kTemplateTolerance &&
         std::fabs(position.y - kTemplateCoordinate) <= kTemplateTolerance;
}

[[nodiscard]] bool IsHumanoid(const Aabb& box) noexcept {
  return box.max.z > kHumanoidMinHeight && box.max.z < kHumanoidMaxHeight &&
         std::fabs(box.min.z) < kHumanoidFootTolerance &&
         std::fabs(box.max.x) < kHumanoidMaxWidth &&
         std::fabs(box.max.y) < kHumanoidMaxWidth;
}

}  // namespace

std::expected<RawEntity, std::error_code> ReadEntity(
    std::uintptr_t address) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kVtable{
      process::ReadPointerAs<EnginePtr>(address)};
  if (!kVtable) {
    return std::unexpected{kVtable.error()};
  }

  const std::expected<Matrix4, std::error_code> kTransform{
      process::ReadAt<Matrix4>(address + kEntityTransform)};
  if (!kTransform) {
    return std::unexpected{kTransform.error()};
  }

  const std::expected<Aabb, std::error_code> kBox{
      process::ReadAt<Aabb>(address + kEntityAabb)};
  if (!kBox) {
    return std::unexpected{kBox.error()};
  }

  return RawEntity{.address = address,
                   .vtable = *kVtable,
                   .transform = *kTransform,
                   .box = *kBox};
}

std::expected<EntityInfo, Rejection> Classify(const RawEntity& raw) noexcept {
  if (raw.vtable == 0 ||
      !raw.transform.IsRigid(kUnitTolerance, kAxisTolerance) ||
      !raw.box.IsValid(kMaxBoxSpan)) {
    return std::unexpected{Rejection::NotAnEntity};
  }

  const Vec3 kPosition{raw.transform.Translation()};
  if (!kPosition.IsFinite() || !kPosition.IsWithin(kMaxWorldCoordinate) ||
      IsTemplateSpawn(kPosition)) {
    return std::unexpected{Rejection::OutsideTheWorld};
  }

  return EntityInfo{.address = raw.address,
                    .vtable = raw.vtable,
                    .transform = raw.transform,
                    .box = raw.box,
                    .pawn = std::nullopt,
                    .humanoid = IsHumanoid(raw.box)};
}

std::optional<PawnInfo> ReadPawn(std::uintptr_t address) noexcept {
  const std::expected<std::int32_t, std::error_code> kArmy{
      process::ReadAt<std::int32_t>(address + kPawnArmy)};
  const std::expected<std::uint8_t, std::error_code> kOwnBit{
      process::ReadAt<std::uint8_t>(address + kPawnOwnArmyBit)};
  if (!kArmy || !kOwnBit || *kArmy < 0 || *kArmy > kMaxArmy) {
    return std::nullopt;
  }
  if (static_cast<std::uint32_t>(*kOwnBit) !=
      (1U << static_cast<std::uint32_t>(*kArmy))) {
    return std::nullopt;
  }

  const std::expected<std::int32_t, std::error_code> kFaction{
      process::ReadAt<std::int32_t>(address + kPawnFaction)};
  if (!kFaction || *kFaction < 0 || *kFaction > kMaxFaction) {
    return std::nullopt;
  }

  const std::uint8_t kDead{
      process::ReadAt<std::uint8_t>(address + kPawnIsDead).value_or(0)};

  return PawnInfo{
      .army = *kArmy,
      .faction = *kFaction,
      .ownArmyBit = *kOwnBit,
      .hostileMask =
          process::ReadAt<std::uint8_t>(address + kPawnHostileMask).value_or(0),
      .reverseHostileMask =
          process::ReadAt<std::uint8_t>(address + kPawnReverseHostile)
              .value_or(0),
      .deadByte = kDead,
      .dead = kDead != 0};
}

}  // namespace afw::games::fc3
