#include <gtest/gtest.h>
#include <windows.h>

#include <afw/games/fc3/fc3.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>

namespace {

constexpr SIZE_T kBufferBytes{4096};

// The engine's pointers are four bytes wide, so the fake has to live where a
// 32-bit value can address it. On a 64-bit test run a plain allocation would
// not.
constexpr std::array<std::uintptr_t, 3> kPreferredBases{0x20000000, 0x30000000,
                                                        0x40000000};

constexpr std::uint32_t kScheduler{0x000};
constexpr std::uint32_t kGroupArray{0x040};
constexpr std::uint32_t kGroupOne{0x080};
constexpr std::uint32_t kGroupTwo{0x0C0};
constexpr std::uint32_t kEntityGroupOne{0x100};
constexpr std::uint32_t kEntityGroupTwo{0x140};
constexpr std::uint32_t kEntityArrayOne{0x200};
constexpr std::uint32_t kEntityArrayTwo{0x240};

constexpr std::uint32_t kSchedulerGroups{0x20};
constexpr std::uint32_t kSchedulerGroupCount{0x24};
constexpr std::uint32_t kGroupEntityGroup{0x08};
constexpr std::uint32_t kEntityGroupArray{0x04};
constexpr std::uint32_t kEntityGroupCount{0x08};

constexpr std::uint32_t kEntitiesOne{7};
constexpr std::uint32_t kEntitiesTwo{5};
constexpr std::uint32_t kMaxGroups{256};

class Fc3CensusUnit : public ::testing::Test {
 protected:
  void SetUp() override {
    for (const std::uintptr_t kBase : kPreferredBases) {
      page_ = VirtualAlloc(
          /*lpAddress        = */ std::bit_cast<LPVOID>(kBase),
          /*dwSize           = */ kBufferBytes,
          /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
          /*flProtect        = */ PAGE_READWRITE);
      if (page_ != nullptr) {
        break;
      }
    }
    if (page_ == nullptr) {
      GTEST_SKIP() << "no free reservation below 4 GiB for the fake scheduler";
    }
    base_ = static_cast<std::uint32_t>(std::bit_cast<std::uintptr_t>(page_));

    Poke(kScheduler + kSchedulerGroups, At(kGroupArray));
    Poke(kScheduler + kSchedulerGroupCount, 2);

    Poke(kGroupArray, At(kGroupOne));
    Poke(kGroupArray + sizeof(std::uint32_t), At(kGroupTwo));

    Poke(kGroupOne + kGroupEntityGroup, At(kEntityGroupOne));
    Poke(kGroupTwo + kGroupEntityGroup, At(kEntityGroupTwo));

    Poke(kEntityGroupOne + kEntityGroupArray, At(kEntityArrayOne));
    Poke(kEntityGroupOne + kEntityGroupCount, kEntitiesOne);

    Poke(kEntityGroupTwo + kEntityGroupArray, At(kEntityArrayTwo));
    Poke(kEntityGroupTwo + kEntityGroupCount, kEntitiesTwo);
  }

  void TearDown() override {
    if (page_ != nullptr) {
      VirtualFree(
          /*lpAddress  = */ page_,
          /*dwSize     = */ 0,
          /*dwFreeType = */ MEM_RELEASE);
    }
  }

  void Poke(std::uint32_t offset, std::uint32_t value) {
    std::memcpy(static_cast<std::byte*>(page_) + offset, &value, sizeof(value));
  }

  [[nodiscard]] std::uint32_t At(std::uint32_t offset) const {
    return base_ + offset;
  }

  [[nodiscard]] static afw::games::fc3::EntityCensus Walk(
      std::uint32_t scheduler) {
    afw::games::fc3::TakeCensus(scheduler);
    afw::games::fc3::PublishCensus();
    return afw::games::fc3::LastCensus();
  }

  LPVOID page_{nullptr};
  std::uint32_t base_{0};
};

}  // namespace

TEST_F(Fc3CensusUnit, CountsGroupsAndTheirEntities) {
  const afw::games::fc3::EntityCensus kCensus{Walk(At(kScheduler))};

  EXPECT_EQ(kCensus.groups, 2U);
  EXPECT_EQ(kCensus.entities, kEntitiesOne + kEntitiesTwo);
  EXPECT_EQ(kCensus.skipped, 0U);
}

TEST_F(Fc3CensusUnit, EveryFrameIsCounted) {
  const std::uint32_t kBefore{afw::games::fc3::LastCensus().frames};

  std::ignore = Walk(At(kScheduler));
  const afw::games::fc3::EntityCensus kCensus{Walk(At(kScheduler))};

  EXPECT_EQ(kCensus.frames, kBefore + 2);
}

TEST_F(Fc3CensusUnit, SeveralPassesInOneFrameAccumulate) {
  afw::games::fc3::TakeCensus(At(kScheduler));
  afw::games::fc3::TakeCensus(At(kScheduler));
  afw::games::fc3::PublishCensus();

  const afw::games::fc3::EntityCensus kCensus{afw::games::fc3::LastCensus()};

  EXPECT_EQ(kCensus.calls, 2U);
  EXPECT_EQ(kCensus.groups, 4U);
  EXPECT_EQ(kCensus.entities, 2 * (kEntitiesOne + kEntitiesTwo));
}

TEST_F(Fc3CensusUnit, PublishingClearsTheAccumulator) {
  std::ignore = Walk(At(kScheduler));
  afw::games::fc3::PublishCensus();

  const afw::games::fc3::EntityCensus kCensus{afw::games::fc3::LastCensus()};

  EXPECT_EQ(kCensus.calls, 0U);
  EXPECT_EQ(kCensus.entities, 0U);
}

TEST_F(Fc3CensusUnit, AnUnreadableSchedulerCountsNothing) {
  const afw::games::fc3::EntityCensus kCensus{Walk(0)};

  EXPECT_EQ(kCensus.groups, 0U);
  EXPECT_EQ(kCensus.entities, 0U);
}

TEST_F(Fc3CensusUnit, AGroupWithoutAnEntityGroupIsSkipped) {
  Poke(kGroupTwo + kGroupEntityGroup, 0);

  const afw::games::fc3::EntityCensus kCensus{Walk(At(kScheduler))};

  EXPECT_EQ(kCensus.groups, 1U);
  EXPECT_EQ(kCensus.entities, kEntitiesOne);
}

TEST_F(Fc3CensusUnit, AnEmptyGroupIsSkipped) {
  Poke(kEntityGroupTwo + kEntityGroupCount, 0);

  const afw::games::fc3::EntityCensus kCensus{Walk(At(kScheduler))};

  EXPECT_EQ(kCensus.groups, 1U);
  EXPECT_EQ(kCensus.entities, kEntitiesOne);
}

TEST_F(Fc3CensusUnit, AnImplausibleGroupCountIsCapped) {
  constexpr std::uint32_t kAbsurd{100000};
  Poke(kScheduler + kSchedulerGroupCount, kAbsurd);

  const afw::games::fc3::EntityCensus kCensus{Walk(At(kScheduler))};

  EXPECT_EQ(kCensus.skipped, kAbsurd - kMaxGroups);
}
