#include <windows.h>

#include <afw/games/fc3/config.hpp>
#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/process/memory.hpp>
#include <array>
#include <bit>
#include <cstddef>
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
      ReadPointer(report.viewportAddress)};
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
      ReadPointer(report.containerAddress)};
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
      ReadPointer(report.container + kContainerManagerSlot)};
  if (!kManagerSlot) {
    return;
  }

  const std::expected<std::uintptr_t, std::error_code> kManager{
      ReadPointer(*kManagerSlot)};
  if (!kManager) {
    return;
  }
  report.manager = *kManager;

  const std::expected<std::uintptr_t, std::error_code> kViewBase{
      ReadPointer(report.manager + kManagerViewSlot)};
  if (!kViewBase || *kViewBase == 0) {
    return;
  }
  report.cameraView = *kViewBase + kViewOffset;
}

// A full open-addressed table reports every further entity as already seen, so
// the tail of the frame would be dropped: keep the load factor near a quarter.
constexpr std::size_t kSeenSlots{8192};
// Engine allocations are 16-byte aligned, so the low bits carry no information.
constexpr std::uint32_t kAlignmentBits{4};

// The same entity turns up in several update passes; inspect it once a frame.
[[nodiscard]] std::array<EnginePtr, kSeenSlots>& Seen() noexcept {
  static std::array<EnginePtr, kSeenSlots> seen{};
  return seen;
}

[[nodiscard]] bool AlreadySeen(std::uintptr_t entity) noexcept {
  const auto kKey{static_cast<EnginePtr>(entity)};
  std::size_t slot{(kKey >> kAlignmentBits) % kSeenSlots};

  for (std::size_t probe{0}; probe < kSeenSlots; ++probe) {
    EnginePtr& cell{Seen().at(slot)};
    if (cell == 0) {
      cell = kKey;
      return false;
    }
    if (cell == kKey) {
      return true;
    }
    slot = (slot + 1) % kSeenSlots;
  }

  return true;
}

EntitySnapshot& PendingSnapshot() noexcept {
  static EntitySnapshot snapshot{};
  return snapshot;
}

EntitySnapshot& PublishedSnapshot() noexcept {
  static EntitySnapshot snapshot{};
  return snapshot;
}

EntityCensus& Pending() noexcept {
  static EntityCensus census{};
  return census;
}

EntityCensus& Published() noexcept {
  static EntityCensus census{};
  return census;
}

const hook::VmtHook*& BoundUpdate() noexcept {
  static const hook::VmtHook* hook{nullptr};
  return hook;
}

[[nodiscard]] std::uint32_t CollectGroup(std::uintptr_t group) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kEntityGroup{
      ReadPointer(group + kGroupEntityGroup)};
  if (!kEntityGroup || *kEntityGroup == 0) {
    return 0;
  }

  const std::expected<std::uintptr_t, std::error_code> kArray{
      ReadPointer(*kEntityGroup + kEntityGroupArray)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(*kEntityGroup + kEntityGroupCount)};
  if (!kArray || !kCount || *kArray == 0 || *kCount > kMaxGroupEntities) {
    return 0;
  }

  EntitySnapshot& snapshot{PendingSnapshot()};

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    if (snapshot.count >= kMaxEntities) {
      snapshot.dropped += *kCount - index;
      break;
    }

    const std::expected<std::uintptr_t, std::error_code> kEntity{
        ReadPointer(*kArray + (index * sizeof(EnginePtr)))};
    if (!kEntity || *kEntity == 0 || AlreadySeen(*kEntity)) {
      continue;
    }

    EntityInfo info{};
    const Verdict kVerdict{InspectEntity(*kEntity, info)};
    if (kVerdict == Verdict::NotEntity) {
      ++snapshot.notEntities;
      continue;
    }
    if (kVerdict != Verdict::Stored) {
      continue;
    }

    snapshot.items.at(snapshot.count) = info;
    ++snapshot.count;
    if (info.pawn) {
      ++snapshot.pawns;
    }
    if (info.humanoid) {
      ++snapshot.humanoids;
    }
  }

  return *kCount;
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

void** RunAllGroupSlot() noexcept {
  return std::bit_cast<void**>(Resolve(kRunAllGroupSlotAddress));
}

void TakeCensus(std::uintptr_t scheduler) noexcept {
  EntityCensus& census{Pending()};
  ++census.calls;

  const std::expected<std::uintptr_t, std::error_code> kGroups{
      ReadPointer(scheduler + kSchedulerGroups)};
  const std::expected<std::uint32_t, std::error_code> kGroupCount{
      process::ReadAt<std::uint32_t>(scheduler + kSchedulerGroupCount)};
  if (!kGroups || !kGroupCount || *kGroups == 0) {
    return;
  }

  const std::uint32_t kWanted{*kGroupCount};
  const std::uint32_t kVisited{kWanted < kMaxGroups ? kWanted : kMaxGroups};
  census.skipped += kWanted - kVisited;

  for (std::uint32_t index{0}; index < kVisited; ++index) {
    const std::expected<std::uintptr_t, std::error_code> kGroup{
        ReadPointer(*kGroups + (index * sizeof(EnginePtr)))};
    if (!kGroup || *kGroup == 0) {
      continue;
    }

    const std::uint32_t kEntities{CollectGroup(*kGroup)};
    if (kEntities == 0) {
      continue;
    }

    ++census.groups;
    census.entities += kEntities;
  }
}

void PublishCensus() noexcept {
  EntityCensus census{Pending()};
  census.frames = Published().frames + 1;

  Published() = census;
  Pending() = EntityCensus{};

  PublishedSnapshot() = PendingSnapshot();
  PendingSnapshot().count = 0;
  PendingSnapshot().dropped = 0;
  PendingSnapshot().pawns = 0;
  PendingSnapshot().humanoids = 0;
  PendingSnapshot().notEntities = 0;
  Seen().fill(0);
  process::ForgetReadableRegions();
}

EntityCensus LastCensus() noexcept { return Published(); }

const EntitySnapshot& LastSnapshot() noexcept { return PublishedSnapshot(); }

UpdateBinding::UpdateBinding(const hook::VmtHook& hook) noexcept {
  BoundUpdate() = &hook;
}

UpdateBinding::~UpdateBinding() { BoundUpdate() = nullptr; }

RunAllGroupFn OriginalRunAllGroup() noexcept {
  const hook::VmtHook* const kHook{BoundUpdate()};
  if (kHook == nullptr) {
    return nullptr;
  }

  return kHook->Original<RunAllGroupFn>();
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
