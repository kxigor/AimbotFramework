#include <windows.h>

#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/process/memory.hpp>
#include <cmath>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

namespace {

constexpr PCWSTR kModuleName{L"FC3_d3d11.dll"};
constexpr std::uintptr_t kGhidraImageBase{0x10000000};

constexpr std::uintptr_t kCameraManagerContainer{0x11e137b4};
constexpr std::uintptr_t kViewportBlock{0x11e5a654};

constexpr std::uintptr_t kContainerManagerSlot{0x08};
constexpr std::uintptr_t kContainerLiveFlag{0x0C};
constexpr std::uintptr_t kManagerViewSlot{0x04};
constexpr std::uintptr_t kViewOffset{0x18};

constexpr std::uintptr_t kViewPosition{0x04};
constexpr std::uintptr_t kViewVerticalFov{0x2C};
constexpr std::uintptr_t kViewForward{0x64};
constexpr std::uintptr_t kViewUp{0x70};

constexpr std::uintptr_t kViewportWidth{0x10};
constexpr std::uintptr_t kViewportHeight{0x14};

constexpr float kUnitTolerance{0.05F};
constexpr float kMinFov{0.1F};
constexpr float kMaxFov{3.0F};

[[nodiscard]] bool IsUnitLength(const Triple& axis) noexcept {
  const float kLength{std::sqrt((axis.at(0) * axis.at(0)) +
                                (axis.at(1) * axis.at(1)) +
                                (axis.at(2) * axis.at(2)))};

  return std::fabs(kLength - 1.0F) < kUnitTolerance;
}

[[nodiscard]] Triple Cross(const Triple& lhs, const Triple& rhs) noexcept {
  return Triple{(lhs.at(1) * rhs.at(2)) - (lhs.at(2) * rhs.at(1)),
                (lhs.at(2) * rhs.at(0)) - (lhs.at(0) * rhs.at(2)),
                (lhs.at(0) * rhs.at(1)) - (lhs.at(1) * rhs.at(0))};
}

void FillViewport(Diagnostics& report) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kBlock{
      process::ReadAt<std::uintptr_t>(report.viewportAddress)};
  if (!kBlock) {
    return;
  }
  report.viewportBlock = *kBlock;

  const std::expected<std::int32_t, std::error_code> kWidth{
      process::ReadAt<std::int32_t>(report.viewportBlock + kViewportWidth)};
  const std::expected<std::int32_t, std::error_code> kHeight{
      process::ReadAt<std::int32_t>(report.viewportBlock + kViewportHeight)};
  if (!kWidth || !kHeight) {
    return;
  }

  report.viewportWidth = *kWidth;
  report.viewportHeight = *kHeight;
}

void FillCameraChain(Diagnostics& report) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kContainer{
      process::ReadAt<std::uintptr_t>(report.containerAddress)};
  if (!kContainer) {
    return;
  }
  report.container = *kContainer;

  const std::expected<std::int32_t, std::error_code> kLive{
      process::ReadAt<std::int32_t>(report.container + kContainerLiveFlag)};
  if (!kLive || *kLive == 0) {
    return;
  }
  report.liveFlag = *kLive;

  const std::expected<std::uintptr_t, std::error_code> kManagerSlot{
      process::ReadAt<std::uintptr_t>(report.container +
                                      kContainerManagerSlot)};
  if (!kManagerSlot) {
    return;
  }

  const std::expected<std::uintptr_t, std::error_code> kManager{
      process::ReadAt<std::uintptr_t>(*kManagerSlot)};
  if (!kManager) {
    return;
  }
  report.manager = *kManager;

  const std::expected<std::uintptr_t, std::error_code> kViewBase{
      process::ReadAt<std::uintptr_t>(report.manager + kManagerViewSlot)};
  if (!kViewBase || *kViewBase == 0) {
    return;
  }
  report.cameraView = *kViewBase + kViewOffset;
}

}  // namespace

std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kBase{
      process::ModuleBase(kModuleName)};
  if (!kBase) {
    return 0;
  }

  return *kBase + (ghidraAddress - kGhidraImageBase);
}

Diagnostics ReadDiagnostics() noexcept {
  Diagnostics report{};

  const std::expected<std::uintptr_t, std::error_code> kBase{
      process::ModuleBase(kModuleName)};
  if (!kBase) {
    return report;
  }

  report.moduleBase = *kBase;
  report.containerAddress = Resolve(kCameraManagerContainer);
  report.viewportAddress = Resolve(kViewportBlock);

  FillViewport(report);
  FillCameraChain(report);

  return report;
}

std::expected<CameraBasis, std::error_code> ReadCamera() noexcept {
  const Diagnostics kReport{ReadDiagnostics()};
  if (kReport.cameraView == 0) {
    return std::unexpected{make_error_code(Error::CameraUnavailable)};
  }
  const std::expected<std::uintptr_t, std::error_code> kView{
      kReport.cameraView};

  const std::expected<Triple, std::error_code> kPosition{
      process::ReadAt<Triple>(*kView + kViewPosition)};
  const std::expected<Triple, std::error_code> kForward{
      process::ReadAt<Triple>(*kView + kViewForward)};
  const std::expected<Triple, std::error_code> kUp{
      process::ReadAt<Triple>(*kView + kViewUp)};
  const std::expected<float, std::error_code> kFov{
      process::ReadAt<float>(*kView + kViewVerticalFov)};
  if (!kPosition || !kForward || !kUp || !kFov) {
    return std::unexpected{make_error_code(Error::CameraNotFound)};
  }

  if (!IsUnitLength(*kForward) || !IsUnitLength(*kUp) || *kFov < kMinFov ||
      *kFov > kMaxFov) {
    return std::unexpected{make_error_code(Error::CameraNotSane)};
  }

  return CameraBasis{.position = *kPosition,
                     .forward = *kForward,
                     .upAxis = *kUp,
                     .right = Cross(*kUp, *kForward),
                     .fovRadians = *kFov};
}

std::expected<Viewport, std::error_code> ReadViewport() noexcept {
  const Diagnostics kReport{ReadDiagnostics()};
  if (kReport.viewportWidth <= 0 || kReport.viewportHeight <= 0) {
    return std::unexpected{make_error_code(Error::ViewportNotSane)};
  }

  return Viewport{.width = static_cast<float>(kReport.viewportWidth),
                  .height = static_cast<float>(kReport.viewportHeight)};
}

}  // namespace afw::games::fc3
