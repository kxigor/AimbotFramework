#pragma once

#include <afw/abi/abi.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <afw/hook/hook.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

namespace afw::games::fc3 {

using RunAllGroupFn = void(AFW_ENGINE_CALL*)(void* scheduler);

[[nodiscard]] void** RunAllGroupSlot() noexcept;

// Called from inside the engine's own entity update, once per pass. Several
// passes make one generation.
void TakeCensus(std::uintptr_t scheduler) noexcept;

// Called once per presented frame: hands the reader whatever generation is
// complete and asks the next walk to start a new one.
void PublishCensus() noexcept;

// How much walking the last generation took, which is what tells a world that
// is empty from one that was never walked.
struct EntityCensus {
  std::uint32_t frames{};
  std::uint32_t calls{};
  std::uint32_t groups{};
  std::uint32_t entities{};
  std::uint32_t skipped{};
};

[[nodiscard]] EntityCensus LastCensus() noexcept;

// Holds the hook the census reads its original through, for as long as the hook
// is installed.
class UpdateBinding {
 public:
  explicit UpdateBinding(const hook::Hook& hook) noexcept;

  UpdateBinding(const UpdateBinding&) = delete;
  UpdateBinding& operator=(const UpdateBinding&) = delete;

  UpdateBinding(UpdateBinding&&) = delete;
  UpdateBinding& operator=(UpdateBinding&&) = delete;

  ~UpdateBinding();
};

[[nodiscard]] RunAllGroupFn OriginalRunAllGroup() noexcept;

constexpr std::size_t kMaxEntities{4096};

constexpr std::size_t kRejectedSamples{8};

constexpr std::size_t kGroupTypes{32};

// One world as the last complete walk found it, with a tally of everything the
// walk threw away beside it: an NPC standing in front of you and not drawn has
// to be traceable to the exact test that dropped it.
struct EntitySnapshot {
  void ForgetCounts() noexcept {
    count = 0;
    dropped = 0;
    pawns = 0;
    humanoids = 0;
    unreadable = 0;
    foreignClass = 0;
    viaComponent = 0;
    regionQueries = 0;
    regionHits = 0;
    rejected.fill(0);
    sampled = 0;
    notRigidSample.fill(0);
    acceptedByType.fill(0);
    droppedByType.fill(0);
  }

  void CountByType(std::uint32_t typeBit, bool accepted) noexcept {
    if (typeBit >= kGroupTypes) {
      return;
    }
    if (accepted) {
      ++acceptedByType.at(typeBit);
      return;
    }

    ++droppedByType.at(typeBit);
  }

  void SampleNotRigid(std::uintptr_t address) noexcept {
    if (sampled < kRejectedSamples) {
      notRigidSample.at(sampled) = address;
      ++sampled;
    }
  }

  std::uint32_t count{};
  std::uint32_t dropped{};
  std::uint32_t pawns{};
  std::uint32_t humanoids{};
  std::uint32_t unreadable{};
  std::uint32_t foreignClass{};
  std::uint32_t viaComponent{};
  std::uint32_t regionQueries{};
  std::uint32_t regionHits{};
  std::array<std::uint32_t, kRejectionCount> rejected{};
  std::uint32_t sampled{};
  std::array<std::uintptr_t, kRejectedSamples> notRigidSample{};
  std::array<std::uint32_t, kGroupTypes> acceptedByType{};
  std::array<std::uint32_t, kGroupTypes> droppedByType{};
  std::array<EntityInfo, kMaxEntities> items{};
};

// The reader owns what this returns until the next PublishCensus, so it may be
// held across a whole frame's drawing.
[[nodiscard]] const EntitySnapshot& LastSnapshot() noexcept;

}  // namespace afw::games::fc3
