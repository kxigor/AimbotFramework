#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/hook/patch.hpp>
#include <afw/math/angle.hpp>
#include <afw/process/memory.hpp>
#include <afw/process/scan.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] bool ReproducesTheView(std::uintptr_t angles,
                                     const Vec3& forward) noexcept {
  const std::expected<float, std::error_code> kPitch{
      process::ReadAt<float>(angles + kViewAnglePitch)};
  const std::expected<float, std::error_code> kYaw{
      process::ReadAt<float>(angles + kViewAngleYaw)};
  if (!kPitch || !kYaw || !std::isfinite(*kPitch) || !std::isfinite(*kYaw)) {
    return false;
  }

  const ViewAngles kWanted{ViewAnglesTo(forward)};

  return std::fabs(math::WrapAngle(*kPitch - kWanted.pitch)) <
             kViewAngleAgreement &&
         std::fabs(math::WrapAngle(*kYaw - kWanted.yaw)) < kViewAngleAgreement;
}

}  // namespace

ViewAngles ViewAnglesTo(const Vec3& direction) noexcept {
  const Vec3 kUnit{direction.Normalised()};

  return ViewAngles{
      .yaw = math::WrapAngle(std::atan2(kUnit.y, kUnit.x) - math::kQuarterTurn),
      .pitch = std::asin(std::clamp(kUnit.z, -1.0F, 1.0F))};
}

std::optional<std::uintptr_t> FindViewAngles(std::uintptr_t playerEntity,
                                             const Vec3& forward) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kArray{
      process::ReadPointerAs<EnginePtr>(playerEntity + kEntityComponentArray)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(playerEntity + kEntityComponentCount)};
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

    const std::expected<std::uintptr_t, std::error_code> kState{
        process::ReadPointerAs<EnginePtr>(*kComponent + kCameraState)};
    if (!kState || *kState == 0) {
      continue;
    }
    if (ReproducesTheView(*kState + kCameraAnglesA, forward)) {
      return *kState;
    }
  }

  return std::nullopt;
}

std::optional<ViewAngles> ReadViewAngles(std::uintptr_t state) noexcept {
  const std::expected<float, std::error_code> kPitch{
      process::ReadAt<float>(state + kCameraAnglesA + kViewAnglePitch)};
  const std::expected<float, std::error_code> kYaw{
      process::ReadAt<float>(state + kCameraAnglesA + kViewAngleYaw)};
  if (!kPitch || !kYaw || !std::isfinite(*kPitch) || !std::isfinite(*kYaw)) {
    return std::nullopt;
  }

  return ViewAngles{.yaw = *kYaw, .pitch = *kPitch};
}

bool WriteViewAngles(std::uintptr_t state, const ViewAngles& angles) noexcept {
  if (!std::isfinite(angles.pitch) || !std::isfinite(angles.yaw)) {
    return false;
  }

  bool written{false};
  for (const std::uintptr_t kBlock :
       {kCameraAnglesA, kCameraAnglesCommit, kCameraAnglesSource}) {
    const std::uintptr_t kAngles{state + kBlock};
    if (process::WriteAt<float>(kAngles + kViewAnglePitch, angles.pitch) &&
        process::WriteAt<float>(kAngles + kViewAngleYaw, angles.yaw)) {
      written = true;
    }
  }

  return written;
}

std::expected<hook::CodePatch, std::error_code> SuppressRecoil() noexcept {
  const std::expected<std::uintptr_t, std::error_code> kSite{
      process::FindPattern(kModuleName, kRecoilStoreSignature)};
  if (!kSite) {
    return std::unexpected{kSite.error()};
  }

  constexpr std::array<std::uint8_t, 5> kSkip{0x83, 0xC4, 0x04, 0x90, 0x90};

  return hook::CodePatch::Apply(*kSite + kRecoilStorePatchOffset, kSkip);
}

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

}  // namespace afw::games::fc3
