#include <afw/games/fc3/camera.hpp>
#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/internal/components.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <afw/games/fc3/module.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/direction.hpp>
#include <afw/process/memory.hpp>
#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

math::ViewAngles ViewAnglesTo(const math::Vec3& direction) noexcept {
  const math::ViewAngles kWorld{math::AnglesOf(direction)};

  return math::ViewAngles{
      .yaw = math::WrapAngle(kWorld.yaw - math::kQuarterTurn),
      .pitch = kWorld.pitch};
}

std::optional<math::ViewAngles> ReadViewAngles(std::uintptr_t state) noexcept {
  const Engine kAngles{state + kCameraAnglesA};
  const std::optional<float> kPitch{kAngles.Field<float>(kViewAnglePitch)};
  const std::optional<float> kYaw{kAngles.Field<float>(kViewAngleYaw)};
  if (!kPitch || !kYaw || !std::isfinite(*kPitch) || !std::isfinite(*kYaw)) {
    return std::nullopt;
  }

  return math::ViewAngles{.yaw = *kYaw, .pitch = *kPitch};
}

bool ReproducesTheView(std::uintptr_t state,
                       const math::Vec3& forward) noexcept {
  const std::optional<math::ViewAngles> kStanding{ReadViewAngles(state)};
  if (!kStanding) {
    return false;
  }

  const math::ViewAngles kWanted{ViewAnglesTo(forward)};

  return std::fabs(math::WrapAngle(kStanding->pitch - kWanted.pitch)) <
             kViewAngleAgreement &&
         std::fabs(math::WrapAngle(kStanding->yaw - kWanted.yaw)) <
             kViewAngleAgreement;
}

std::optional<std::uintptr_t> FindViewAngles(
    std::uintptr_t playerEntity, const math::Vec3& forward) noexcept {
  return FirstComponent(
      playerEntity,
      [&forward](std::uintptr_t part) -> std::optional<std::uintptr_t> {
        const std::optional<std::uintptr_t> kState{
            Engine{part}.Pointer(kCameraState)};
        if (!kState || !ReproducesTheView(*kState, forward)) {
          return std::nullopt;
        }

        return kState;
      });
}

bool WriteViewAngles(std::uintptr_t state,
                     const math::ViewAngles& angles) noexcept {
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

std::expected<CameraBasis, std::error_code> ReadCamera() noexcept {
  const Diagnostics kReport{Diagnostics::Read()};
  if (kReport.cameraView == 0) {
    return std::unexpected{make_error_code(Error::CameraUnavailable)};
  }

  const Engine kView{kReport.cameraView};
  const std::optional<math::Vec3> kPosition{
      kView.Field<math::Vec3>(kViewPosition)};
  const std::optional<math::Vec3> kForward{
      kView.Field<math::Vec3>(kViewForward)};
  const std::optional<math::Vec3> kUp{kView.Field<math::Vec3>(kViewUp)};
  const std::optional<float> kFov{kView.Field<float>(kViewVerticalFov)};
  const std::optional<float> kFovHorizontal{
      kView.Field<float>(kViewHorizontalFov)};
  if (!kPosition || !kForward || !kUp || !kFov || !kFovHorizontal) {
    return std::unexpected{make_error_code(Error::CameraNotFound)};
  }

  if (!kForward->IsUnitLength(kUnitTolerance) ||
      !kUp->IsUnitLength(kUnitTolerance) || *kFov < kMinFov ||
      *kFov > kMaxFov || *kFovHorizontal < kMinFov ||
      *kFovHorizontal > kMaxFov) {
    return std::unexpected{make_error_code(Error::CameraNotSane)};
  }

  const std::optional<math::Vec3> kRight{kView.Field<math::Vec3>(kViewRight)};

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
