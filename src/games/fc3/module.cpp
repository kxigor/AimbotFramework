#include <windows.h>

#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/module.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <afw/games/fc3/module.hpp>
#include <afw/process/memory.hpp>
#include <cstdint>
#include <expected>
#include <optional>
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
  const std::optional<std::uintptr_t> kBlock{
      Engine{report.viewportAddress}.Pointer(0)};
  if (!kBlock) {
    return;
  }
  report.viewportBlock = *kBlock;

  const Engine kViewport{report.viewportBlock};
  const std::optional<std::int32_t> kWidth{
      kViewport.Field<std::int32_t>(kViewportWidth)};
  const std::optional<std::int32_t> kHeight{
      kViewport.Field<std::int32_t>(kViewportHeight)};
  if (!kWidth || !kHeight) {
    return;
  }

  report.viewportWidth = *kWidth;
  report.viewportHeight = *kHeight;
}

void FillCameraChain(Diagnostics& report) noexcept {
  const std::optional<std::uintptr_t> kContainer{
      Engine{report.containerAddress}.Pointer(0)};
  if (!kContainer) {
    return;
  }
  report.container = *kContainer;

  const Engine kLiveContainer{report.container};
  const std::optional<std::int32_t> kLive{
      kLiveContainer.Field<std::int32_t>(kContainerLiveFlag)};
  if (!kLive || *kLive == 0) {
    return;
  }
  report.liveFlag = *kLive;

  const std::optional<std::uintptr_t> kManagerSlot{
      kLiveContainer.Pointer(kContainerManagerSlot)};
  if (!kManagerSlot) {
    return;
  }

  const std::optional<std::uintptr_t> kManager{
      Engine{*kManagerSlot}.Pointer(0)};
  if (!kManager) {
    return;
  }
  report.manager = *kManager;

  const std::optional<std::uintptr_t> kViewBase{
      Engine{report.manager}.Pointer(kManagerViewSlot)};
  if (!kViewBase) {
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

}  // namespace afw::games::fc3
