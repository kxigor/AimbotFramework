#include <afw/games/fc3/config.hpp>
#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/process/memory.hpp>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

namespace {

// The army fields sit past the 0xDC of a plain CEntity, so nothing is accepted
// unless the engine's invariant holds: the cached bit is exactly 1 << army.
void FillPawnFields(std::uintptr_t entity, EntityInfo& info) noexcept {
  const std::expected<std::int32_t, std::error_code> kArmy{
      process::ReadAt<std::int32_t>(entity + kPawnArmy)};
  const std::expected<std::uint8_t, std::error_code> kOwnBit{
      process::ReadAt<std::uint8_t>(entity + kPawnOwnArmyBit)};
  if (!kArmy || !kOwnBit || *kArmy < 0 || *kArmy > kMaxArmy) {
    return;
  }
  if (static_cast<std::uint32_t>(*kOwnBit) !=
      (1U << static_cast<std::uint32_t>(*kArmy))) {
    return;
  }

  const std::expected<std::int32_t, std::error_code> kFaction{
      process::ReadAt<std::int32_t>(entity + kPawnFaction)};
  if (!kFaction || *kFaction < 0 || *kFaction > kMaxFaction) {
    return;
  }

  info.pawn = true;
  info.army = *kArmy;
  info.faction = *kFaction;
  info.ownArmyBit = *kOwnBit;
  info.hostileMask =
      process::ReadAt<std::uint8_t>(entity + kPawnHostileMask).value_or(0);
  info.reverseHostileMask =
      process::ReadAt<std::uint8_t>(entity + kPawnReverseHostile).value_or(0);

  const std::expected<std::uint8_t, std::error_code> kDead{
      process::ReadAt<std::uint8_t>(entity + kPawnIsDead)};
  if (kDead) {
    info.deadByte = *kDead;
    info.dead = *kDead != 0;
  }
}

[[nodiscard]] bool FillGeometry(std::uintptr_t entity,
                                EntityInfo& info) noexcept {
  const std::expected<Vec3, std::error_code> kAxisX{
      process::ReadAt<Vec3>(entity + kEntityAxisX)};
  const std::expected<Vec3, std::error_code> kAxisY{
      process::ReadAt<Vec3>(entity + kEntityAxisY)};
  const std::expected<Vec3, std::error_code> kAxisZ{
      process::ReadAt<Vec3>(entity + kEntityAxisZ)};
  if (!kAxisX || !kAxisY || !kAxisZ ||
      !IsOrthonormal(*kAxisX, *kAxisY, *kAxisZ)) {
    return false;
  }

  const std::expected<Vec3, std::error_code> kAabbMin{
      process::ReadAt<Vec3>(entity + kEntityAabbMin)};
  const std::expected<Vec3, std::error_code> kAabbMax{
      process::ReadAt<Vec3>(entity + kEntityAabbMax)};
  if (!kAabbMin || !kAabbMax || !IsSaneBox(*kAabbMin, *kAabbMax)) {
    return false;
  }

  info.axisX = *kAxisX;
  info.axisY = *kAxisY;
  info.axisZ = *kAxisZ;
  info.aabbMin = *kAabbMin;
  info.aabbMax = *kAabbMax;
  info.humanoid = IsHumanoidBox(*kAabbMin, *kAabbMax);

  const float kSpan{kAabbMax->z - kAabbMin->z};
  if (kSpan > kMinHeight && kSpan < kMaxHeight) {
    info.height = kSpan;
  }

  return true;
}

}  // namespace

std::expected<CameraBasis, std::error_code> ReadCamera() noexcept {
  const Diagnostics kReport{Diagnostics::Read()};
  if (kReport.cameraView == 0) {
    return std::unexpected{make_error_code(Error::CameraUnavailable)};
  }
  const std::expected<std::uintptr_t, std::error_code> kView{
      kReport.cameraView};

  const std::expected<Vec3, std::error_code> kPosition{
      process::ReadAt<Vec3>(*kView + kViewPosition)};
  const std::expected<Vec3, std::error_code> kForward{
      process::ReadAt<Vec3>(*kView + kViewForward)};
  const std::expected<Vec3, std::error_code> kUp{
      process::ReadAt<Vec3>(*kView + kViewUp)};
  const std::expected<float, std::error_code> kFov{
      process::ReadAt<float>(*kView + kViewVerticalFov)};
  const std::expected<float, std::error_code> kFovHorizontal{
      process::ReadAt<float>(*kView + kViewHorizontalFov)};
  if (!kPosition || !kForward || !kUp || !kFov || !kFovHorizontal) {
    return std::unexpected{make_error_code(Error::CameraNotFound)};
  }

  if (!kForward->IsUnitLength(kUnitTolerance) ||
      !kUp->IsUnitLength(kUnitTolerance) || *kFov < kMinFov ||
      *kFov > kMaxFov || *kFovHorizontal < kMinFov ||
      *kFovHorizontal > kMaxFov) {
    return std::unexpected{make_error_code(Error::CameraNotSane)};
  }

  // The engine keeps its own right vector next to the basis; the cross product
  // is only the fallback.
  const std::expected<Vec3, std::error_code> kRight{
      process::ReadAt<Vec3>(*kView + kViewRight)};

  return CameraBasis{.position = *kPosition,
                     .forward = *kForward,
                     .upAxis = *kUp,
                     .right = (kRight && kRight->IsUnitLength(kUnitTolerance))
                                  ? *kRight
                                  : kForward->Cross(*kUp),
                     .fovRadians = *kFov,
                     .fovHorizontalRadians = *kFovHorizontal};
}

Verdict InspectEntity(std::uintptr_t entity, EntityInfo& info) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kVtable{
      ReadPointer(entity)};
  if (!kVtable || *kVtable == 0) {
    return Verdict::NotEntity;
  }

  if (!FillGeometry(entity, info)) {
    return Verdict::NotEntity;
  }

  const std::expected<Vec3, std::error_code> kPosition{
      process::ReadAt<Vec3>(entity + kEntityPosition)};
  if (!kPosition || !IsInTheWorld(*kPosition)) {
    return Verdict::NotInTheWorld;
  }

  info.address = entity;
  info.vtable = *kVtable;
  info.position = *kPosition;

  if (info.humanoid) {
    FillPawnFields(entity, info);
  }

  return Verdict::Stored;
}

}  // namespace afw::games::fc3
