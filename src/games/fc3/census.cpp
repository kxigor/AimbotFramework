#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/process/memory.hpp>
#include <array>
#include <atomic>
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

struct CensusBuffer {
  EntityCensus census{};
  EntitySnapshot snapshot{};
};

constexpr std::size_t kBuffers{3};

[[nodiscard]] std::array<CensusBuffer, kBuffers>& Buffers() noexcept {
  static std::array<CensusBuffer, kBuffers> buffers{};
  return buffers;
}

[[nodiscard]] std::atomic<std::size_t>& ReadySlot() noexcept {
  static std::atomic<std::size_t> ready{1};
  return ready;
}

[[nodiscard]] std::atomic<bool>& ReadyIsFresh() noexcept {
  static std::atomic<bool> fresh{false};
  return fresh;
}

[[nodiscard]] std::atomic<bool>& GenerationWanted() noexcept {
  static std::atomic<bool> wanted{true};
  return wanted;
}

[[nodiscard]] std::size_t& WritingIndex() noexcept {
  static std::size_t index{0};
  return index;
}

[[nodiscard]] std::size_t& PublishedIndex() noexcept {
  static std::size_t index{2};
  return index;
}

[[nodiscard]] std::uint32_t& PublishedFrames() noexcept {
  static std::uint32_t frames{0};
  return frames;
}

// A generation is everything the engine walked between two presented frames.
// One pass is not a whole world -- several of them together are -- so the
// boundary belongs to the reader, and the writer only acts on the request.
void BeginGeneration() noexcept {
  WritingIndex() =
      ReadySlot().exchange(WritingIndex(), std::memory_order_acq_rel);
  ReadyIsFresh().store(true, std::memory_order_release);

  CensusBuffer& fresh{Buffers().at(WritingIndex())};
  fresh.census = EntityCensus{};
  fresh.snapshot.ForgetCounts();

  Seen().fill(0);
  process::ForgetReadableRegions();
}

const hook::Hook*& BoundUpdate() noexcept {
  static const hook::Hook* hook{nullptr};
  return hook;
}

void StoreEntity(std::uintptr_t address, EntitySnapshot& snapshot) noexcept {
  const std::expected<RawEntity, std::error_code> kRaw{ReadEntity(address)};
  if (!kRaw) {
    ++snapshot.unreadable;
    return;
  }

  std::expected<EntityInfo, Rejection> classified{Classify(*kRaw)};
  if (!classified) {
    ++snapshot.rejected.at(static_cast<std::size_t>(classified.error()));
    if (classified.error() == Rejection::NotRigid) {
      snapshot.SampleNotRigid(address);
    }
    return;
  }

  // Asked of every entity, not only the ones shaped like a person: an animal is
  // a pawn too, and the army invariant is what proves the record real.
  classified->pawn = ReadPawn(address);

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

  EntitySnapshot& snapshot{Buffers().at(WritingIndex()).snapshot};

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
  if (GenerationWanted().exchange(false, std::memory_order_acquire)) {
    BeginGeneration();
  }

  EntityCensus& census{Buffers().at(WritingIndex()).census};
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
  if (ReadyIsFresh().exchange(false, std::memory_order_acquire)) {
    PublishedIndex() =
        ReadySlot().exchange(PublishedIndex(), std::memory_order_acq_rel);
  }

  ++PublishedFrames();
  GenerationWanted().store(true, std::memory_order_release);
}

EntityCensus LastCensus() noexcept {
  EntityCensus census{Buffers().at(PublishedIndex()).census};
  census.frames = PublishedFrames();

  return census;
}

const EntitySnapshot& LastSnapshot() noexcept {
  return Buffers().at(PublishedIndex()).snapshot;
}

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
