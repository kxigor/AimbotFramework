#include <afw/games/fc3/internal/read.hpp>
#include <afw/games/fc3/internal/snapshot.hpp>
#include <afw/games/fc3/snapshot.hpp>
#include <afw/process/memory.hpp>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

extern "C" {
__declspec(dllexport) const void* afwSnapshot{nullptr};
}

namespace afw::games::fc3 {

namespace {

constexpr std::size_t kSeenSlots{8192};
constexpr std::uint32_t kAlignmentBits{4};
constexpr std::size_t kBuffers{3};

struct CensusBuffer {
  EntityCensus census{};
  EntitySnapshot snapshot{};
};

[[nodiscard]] std::array<EnginePtr, kSeenSlots>& Seen() noexcept {
  static std::array<EnginePtr, kSeenSlots> seen{};
  return seen;
}

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

void BeginGeneration() noexcept {
  WritingIndex() =
      ReadySlot().exchange(WritingIndex(), std::memory_order_acq_rel);
  ReadyIsFresh().store(true, std::memory_order_release);

  CensusBuffer& fresh{Buffers().at(WritingIndex())};
  fresh.census = EntityCensus{};
  fresh.snapshot.ForgetCounts();

  Seen().fill(0);
  process::ForgetRegionStats();
  process::ForgetReadableRegions();
}

}  // namespace

EntitySnapshot& WritingSnapshot() noexcept {
  return Buffers().at(WritingIndex()).snapshot;
}

EntityCensus& WritingCensus() noexcept {
  return Buffers().at(WritingIndex()).census;
}

void OpenGenerationIfWanted() noexcept {
  if (GenerationWanted().exchange(false, std::memory_order_acquire)) {
    BeginGeneration();
  }
}

bool AlreadySeen(std::uintptr_t entity) noexcept {
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

void PublishCensus() noexcept {
  if (ReadyIsFresh().exchange(false, std::memory_order_acquire)) {
    PublishedIndex() =
        ReadySlot().exchange(PublishedIndex(), std::memory_order_acq_rel);
  }

  afwSnapshot = &Buffers().at(PublishedIndex()).snapshot;
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

}  // namespace afw::games::fc3
