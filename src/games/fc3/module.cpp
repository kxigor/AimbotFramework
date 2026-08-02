#include <windows.h>

#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/process/memory.hpp>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] std::uintptr_t CachedModuleBase() noexcept {
  static std::uintptr_t base{0};
  if (base != 0) {
    return base;
  }

  const std::expected<std::uintptr_t, std::error_code> kBase{
      process::ModuleBase(kModuleName)};
  if (kBase) {
    base = *kBase;
  }

  return base;
}

void FillViewport(Diagnostics& report) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kBlock{
      process::ReadPointerAs<EnginePtr>(report.viewportAddress)};
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
      process::ReadPointerAs<EnginePtr>(report.containerAddress)};
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
      process::ReadPointerAs<EnginePtr>(report.container +
                                        kContainerManagerSlot)};
  if (!kManagerSlot) {
    return;
  }

  const std::expected<std::uintptr_t, std::error_code> kManager{
      process::ReadPointerAs<EnginePtr>(*kManagerSlot)};
  if (!kManager) {
    return;
  }
  report.manager = *kManager;

  const std::expected<std::uintptr_t, std::error_code> kViewBase{
      process::ReadPointerAs<EnginePtr>(report.manager + kManagerViewSlot)};
  if (!kViewBase || *kViewBase == 0) {
    return;
  }
  report.cameraView = *kViewBase + kViewOffset;
}

}  // namespace

std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept {
  const std::uintptr_t kBase{CachedModuleBase()};
  if (kBase == 0) {
    return 0;
  }

  return kBase + (ghidraAddress - kGhidraImageBase);
}

Diagnostics Diagnostics::Read() noexcept {
  Diagnostics report{};

  const std::uintptr_t kBase{CachedModuleBase()};
  if (kBase == 0) {
    return report;
  }

  report.moduleBase = kBase;
  report.containerAddress = Resolve(kCameraManagerContainer);
  report.viewportAddress = Resolve(kViewportBlock);

  FillViewport(report);
  FillCameraChain(report);

  return report;
}

std::expected<Viewport, std::error_code> ReadViewport() noexcept {
  const Diagnostics kReport{Diagnostics::Read()};
  if (kReport.viewportWidth <= 0 || kReport.viewportHeight <= 0) {
    return std::unexpected{make_error_code(Error::ViewportNotSane)};
  }

  return Viewport{.width = static_cast<float>(kReport.viewportWidth),
                  .height = static_cast<float>(kReport.viewportHeight)};
}

}  // namespace afw::games::fc3
