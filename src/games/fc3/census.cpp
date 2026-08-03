#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/process/memory.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

namespace {

constexpr std::size_t kSeenSlots{8192};
constexpr std::uint32_t kAlignmentBits{4};

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

const hook::Hook*& BoundUpdate() noexcept {
  static const hook::Hook* hook{nullptr};
  return hook;
}

void StoreEntity(std::uintptr_t address, EntitySnapshot& snapshot) noexcept {
  const std::expected<RawEntity, std::error_code> kRaw{ReadEntity(address)};
  if (!kRaw) {
    ++snapshot.notEntities;
    return;
  }

  std::expected<EntityInfo, Rejection> classified{Classify(*kRaw)};
  if (!classified) {
    if (classified.error() == Rejection::NotAnEntity) {
      ++snapshot.notEntities;
    }
    return;
  }

  if (classified->humanoid) {
    classified->pawn = ReadPawn(address);
  }

  snapshot.items.at(snapshot.count) = *classified;
  ++snapshot.count;
  if (classified->pawn) {
    ++snapshot.pawns;
  }
  if (classified->humanoid) {
    ++snapshot.humanoids;
  }
}

[[nodiscard]] std::uint32_t CollectGroup(std::uintptr_t group) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kEntityGroup{
      process::ReadPointerAs<EnginePtr>(group + kGroupEntityGroup)};
  if (!kEntityGroup || *kEntityGroup == 0) {
    return 0;
  }

  const std::expected<std::uintptr_t, std::error_code> kArray{
      process::ReadPointerAs<EnginePtr>(*kEntityGroup + kEntityGroupArray)};
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
        process::ReadPointerAs<EnginePtr>(*kArray +
                                          (index * sizeof(EnginePtr)))};
    if (!kEntity || *kEntity == 0 || AlreadySeen(*kEntity)) {
      continue;
    }

    StoreEntity(*kEntity, snapshot);
  }

  return *kCount;
}

}  // namespace

void** RunAllGroupSlot() noexcept {
  return std::bit_cast<void**>(Resolve(kRunAllGroupSlotAddress));
}

void TakeCensus(std::uintptr_t scheduler) noexcept {
  EntityCensus& census{Pending()};
  ++census.calls;

  const std::expected<std::uintptr_t, std::error_code> kGroups{
      process::ReadPointerAs<EnginePtr>(scheduler + kSchedulerGroups)};
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
        process::ReadPointerAs<EnginePtr>(*kGroups +
                                          (index * sizeof(EnginePtr)))};
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

  EntitySnapshot& pending{PendingSnapshot()};
  EntitySnapshot& published{PublishedSnapshot()};
  published.count = pending.count;
  published.dropped = pending.dropped;
  published.pawns = pending.pawns;
  published.humanoids = pending.humanoids;
  published.notEntities = pending.notEntities;
  std::copy_n(pending.items.begin(), pending.count, published.items.begin());

  pending.count = 0;
  pending.dropped = 0;
  pending.pawns = 0;
  pending.humanoids = 0;
  pending.notEntities = 0;
  Seen().fill(0);
  process::ForgetReadableRegions();
}

EntityCensus LastCensus() noexcept { return Published(); }

const EntitySnapshot& LastSnapshot() noexcept { return PublishedSnapshot(); }

UpdateBinding::UpdateBinding(const hook::Hook& hook) noexcept {
  BoundUpdate() = &hook;
}

UpdateBinding::~UpdateBinding() { BoundUpdate() = nullptr; }

RunAllGroupFn OriginalRunAllGroup() noexcept {
  const hook::Hook* const kHook{BoundUpdate()};
  if (kHook == nullptr) {
    return nullptr;
  }

  return kHook->Original<RunAllGroupFn>();
}

}  // namespace afw::games::fc3
