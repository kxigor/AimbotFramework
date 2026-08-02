#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/process/memory.hpp>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

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
