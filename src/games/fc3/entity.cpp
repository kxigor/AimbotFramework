#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/process/memory.hpp>
#include <cmath>
#include <cstddef>
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

// The engine's own choice, in its own order: the component's copy when there is
// one, the entity's own only as the fallback.
[[nodiscard]] std::expected<Matrix4, std::error_code> ReadWorldMatrix(
    std::uintptr_t address) noexcept {
  const std::optional<Matrix4> kFromComponent{
      ReadComponentWorldMatrix(address)};
  if (kFromComponent) {
    return *kFromComponent;
  }

  return process::ReadAt<Matrix4>(address + kEntityTransform);
}

// The own-army mask names the army the pawn belongs to, so a record is only
// believable when the two agree. Both agreements exist: one bit for an enlisted
// pawn, every bit for one the engine does not enlist at all.
[[nodiscard]] bool IsConsistentArmy(std::int32_t army,
                                    std::uint8_t ownBit) noexcept {
  if (army == kNoArmy) {
    return ownBit == kNoArmyBit;
  }
  if (army < 0 || army > kMaxArmy) {
    return false;
  }

  return static_cast<std::uint32_t>(ownBit) ==
         (1U << static_cast<std::uint32_t>(army));
}

[[nodiscard]] bool IsHumanoid(const Aabb& box) noexcept {
  return box.max.z > kHumanoidMinHeight && box.max.z < kHumanoidMaxHeight &&
         std::fabs(box.min.z) < kHumanoidFootTolerance &&
         std::fabs(box.max.x) < kHumanoidMaxWidth &&
         std::fabs(box.max.y) < kHumanoidMaxWidth;
}

}  // namespace

std::optional<Matrix4> ReadComponentWorldMatrix(
    std::uintptr_t address) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kArray{
      process::ReadPointerAs<EnginePtr>(address + kEntityComponentArray)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(address + kEntityComponentCount)};
  if (!kArray || !kCount || *kArray == 0 || *kCount > kMaxComponents) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::expected<std::uintptr_t, std::error_code> kComponent{
        process::ReadPointerAs<EnginePtr>(*kArray +
                                          (index * sizeof(EnginePtr)))};
    if (!kComponent || *kComponent == 0) {
      continue;
    }

    const std::expected<Matrix4, std::error_code> kMatrix{
        process::ReadAt<Matrix4>(*kComponent + kComponentWorldMatrix)};
    if (kMatrix && kMatrix->IsRigid(kUnitTolerance, kAxisTolerance)) {
      return *kMatrix;
    }
  }

  return std::nullopt;
}

std::expected<RawEntity, std::error_code> ReadEntity(
    std::uintptr_t address) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kVtable{
      process::ReadPointerAs<EnginePtr>(address)};
  if (!kVtable) {
    return std::unexpected{kVtable.error()};
  }

  const std::expected<Matrix4, std::error_code> kTransform{
      ReadWorldMatrix(address)};
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
  if (raw.vtable == 0) {
    return std::unexpected{Rejection::NoVtable};
  }
  if (!raw.transform.IsRigid(kUnitTolerance, kAxisTolerance)) {
    return std::unexpected{Rejection::NotRigid};
  }
  if (!raw.box.IsValid(kMaxBoxSpan)) {
    return std::unexpected{Rejection::BadBox};
  }

  const Vec3 kPosition{raw.transform.Translation()};
  if (!kPosition.IsFinite()) {
    return std::unexpected{Rejection::NonFinite};
  }
  if (!kPosition.IsWithin(kMaxWorldCoordinate)) {
    return std::unexpected{Rejection::BeyondTheWorld};
  }
  if (IsTemplateSpawn(kPosition)) {
    return std::unexpected{Rejection::TemplateSpawn};
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
  if (!kArmy || !kOwnBit) {
    return std::nullopt;
  }
  if (!IsConsistentArmy(*kArmy, *kOwnBit)) {
    return std::nullopt;
  }

  const std::expected<std::int32_t, std::error_code> kFaction{
      process::ReadAt<std::int32_t>(address + kPawnFaction)};
  if (!kFaction) {
    return std::nullopt;
  }

  const std::uint8_t kDead{
      process::ReadAt<std::uint8_t>(address + kPawnIsDead).value_or(0)};

  return PawnInfo{
      .army = *kArmy,
      .faction = *kFaction,
      .agentType =
          process::ReadAt<std::uint32_t>(address + kPawnAgentType).value_or(0),
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
