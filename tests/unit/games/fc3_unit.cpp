#include <gtest/gtest.h>
#include <windows.h>

#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>
#include <tuple>

namespace {

constexpr SIZE_T kBufferBytes{16384};

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

// Two fake entities, far enough apart that the pawn fields of the first do not
// reach into the second. The offsets are the ones internal/layout.hpp uses.
constexpr std::uint32_t kEntityOne{0x400};
constexpr std::uint32_t kEntityTwo{0x800};

constexpr std::uint32_t kEntityTransform{0x20};
constexpr std::uint32_t kEntityAabb{0x6C};

// The army fields belong to the agent, not the entity: the property registrar
// puts them in a nested CPawnAgent group. Reaching them takes the same walk the
// engine does -- the component whose cached class hash is CAIComponent's, then
// its +0x14.
constexpr std::uint32_t kEntityComponentArray{0x88};
constexpr std::uint32_t kEntityComponentCount{0x8C};
constexpr std::uint32_t kComponentAgent{0x14};

constexpr std::uint32_t kComponentArrayOne{0x1000};
constexpr std::uint32_t kComponentOne{0x1100};
constexpr std::uint32_t kAgentOne{0x2000};

constexpr std::uint32_t kPawnOwnArmyBit{0x280};
constexpr std::uint32_t kPawnArmy{0x284};
constexpr std::uint32_t kPawnFaction{0x290};
constexpr std::uint32_t kPawnAgentType{0x294};

constexpr std::uint32_t kRowBytes{0x10};
constexpr std::uint32_t kVec3Bytes{0x0C};
constexpr std::uintptr_t kWorldWrap{0x10000};

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

    GiveAnAgent(kEntityOne, kComponentArrayOne, kComponentOne, kAgentOne);
  }

  // Only the first entity gets one, so a test can tell the two apart by whether
  // a pawn record appeared at all.
  void GiveAnAgent(std::uint32_t entity, std::uint32_t array,
                   std::uint32_t component, std::uint32_t agent) {
    Poke(entity + kEntityComponentArray, At(array));
    Poke(entity + kEntityComponentCount, 1);
    Poke(array, At(component));
    Poke(component, 0xC0FF0000 + component);
    Poke(component + kComponentAgent, At(agent));
    Poke(agent, 0xA6E70000 + agent);
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

  void PokeFloat(std::uint32_t offset, float value) {
    std::memcpy(static_cast<std::byte*>(page_) + offset, &value, sizeof(value));
  }

  // A rigid identity basis with the position in row 3, and an object-space box
  // around the origin. Everything Classify insists on, and nothing else.
  void PlaceEntity(std::uint32_t entity, float x, float y, float halfWidth,
                   float height) {
    Poke(entity, 0xABCD0000 + entity);

    PokeFloat(entity + kEntityTransform, 1.0F);
    PokeFloat(entity + kEntityTransform + kRowBytes + 4, 1.0F);
    PokeFloat(entity + kEntityTransform + (2 * kRowBytes) + 8, 1.0F);
    PokeFloat(entity + kEntityTransform + (3 * kRowBytes) + 12, 1.0F);

    PokeFloat(entity + kEntityTransform + (3 * kRowBytes), x);
    PokeFloat(entity + kEntityTransform + (3 * kRowBytes) + 4, y);
    PokeFloat(entity + kEntityTransform + (3 * kRowBytes) + 8, 0.0F);

    PokeFloat(entity + kEntityAabb, -halfWidth);
    PokeFloat(entity + kEntityAabb + 4, -halfWidth);
    PokeFloat(entity + kEntityAabb + 8, 0.0F);
    PokeFloat(entity + kEntityAabb + kVec3Bytes, halfWidth);
    PokeFloat(entity + kEntityAabb + kVec3Bytes + 4, halfWidth);
    PokeFloat(entity + kEntityAabb + kVec3Bytes + 8, height);
  }

  [[nodiscard]] std::uint32_t At(std::uint32_t offset) const {
    return base_ + offset;
  }

  // A generation is closed by the walk that follows the frame asking for one,
  // so seeing what a walk found takes a second frame. The engine gets that for
  // free; a test has to spell it out.
  static void Settle(std::uint32_t scheduler) {
    afw::games::fc3::PublishCensus();
    afw::games::fc3::TakeCensus(scheduler);
    afw::games::fc3::PublishCensus();
  }

  [[nodiscard]] static afw::games::fc3::EntityCensus Walk(
      std::uint32_t scheduler) {
    afw::games::fc3::TakeCensus(scheduler);
    Settle(scheduler);
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

  afw::games::fc3::PublishCensus();
  afw::games::fc3::PublishCensus();

  EXPECT_EQ(afw::games::fc3::LastCensus().frames, kBefore + 2);
}

// One pass is not a whole world -- the engine walks it across several -- so a
// generation must add them up. Publishing each pass on its own is what made the
// overlay flicker.
TEST_F(Fc3CensusUnit, SeveralPassesInOneFrameAccumulate) {
  afw::games::fc3::TakeCensus(At(kScheduler));
  afw::games::fc3::TakeCensus(At(kScheduler));
  Settle(At(kScheduler));

  const afw::games::fc3::EntityCensus kCensus{afw::games::fc3::LastCensus()};

  EXPECT_EQ(kCensus.calls, 2U);
  EXPECT_EQ(kCensus.groups, 4U);
  EXPECT_EQ(kCensus.entities, 2 * (kEntitiesOne + kEntitiesTwo));
}

// A frame the engine did not walk keeps the last complete world rather than
// blanking: the reader owns its buffer until a new generation is ready.
TEST_F(Fc3CensusUnit, AFrameWithoutAWalkKeepsTheLastWorld) {
  std::ignore = Walk(At(kScheduler));

  afw::games::fc3::PublishCensus();

  const afw::games::fc3::EntityCensus kCensus{afw::games::fc3::LastCensus()};

  EXPECT_EQ(kCensus.entities, kEntitiesOne + kEntitiesTwo);
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

// Everything below drives the half of the census the ESP actually draws from.
// Before these, no test ever put a non-null pointer in an entity array, so
// StoreEntity and the whole snapshot path ran in no test at all.
class Fc3SnapshotUnit : public Fc3CensusUnit {
 protected:
  void SetUp() override {
    Fc3CensusUnit::SetUp();
    if (page_ == nullptr) {
      return;
    }

    PlaceEntity(kEntityOne, 100.0F, 200.0F, 0.6F, 1.7F);
    PlaceEntity(kEntityTwo, 300.0F, 400.0F, 2.0F, 5.0F);

    Poke(kEntityArrayOne, At(kEntityOne));
    Poke(kEntityArrayTwo, At(kEntityTwo));
  }
};

TEST_F(Fc3SnapshotUnit, PublishesTheEntitiesTheWalkFound) {
  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kSnapshot{
      afw::games::fc3::LastSnapshot()};

  ASSERT_EQ(kSnapshot.count, 2U);
  EXPECT_EQ(kSnapshot.humanoids, 1U);
  EXPECT_EQ(kSnapshot.dropped, 0U);
}

TEST_F(Fc3SnapshotUnit, TellsAHumanoidShapedBoxFromAProp) {
  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kSnapshot{
      afw::games::fc3::LastSnapshot()};
  ASSERT_EQ(kSnapshot.count, 2U);

  EXPECT_TRUE(kSnapshot.items.at(0).humanoid);
  EXPECT_FALSE(kSnapshot.items.at(1).humanoid);
  EXPECT_EQ(kSnapshot.items.at(0).address, At(kEntityOne));
}

TEST_F(Fc3SnapshotUnit, CarriesThePositionThroughUnchanged) {
  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kSnapshot{
      afw::games::fc3::LastSnapshot()};
  ASSERT_EQ(kSnapshot.count, 2U);

  const afw::math::Vec3 kPosition{
      kSnapshot.items.at(0).transform.Translation()};
  EXPECT_FLOAT_EQ(kPosition.x, 100.0F);
  EXPECT_FLOAT_EQ(kPosition.y, 200.0F);
}

TEST_F(Fc3SnapshotUnit, GivesAHumanoidItsPawnRecordWhenTheFieldsAreInRange) {
  Poke(kAgentOne + kPawnArmy, 2);
  Poke(kAgentOne + kPawnFaction, 1);
  Poke(kAgentOne + kPawnAgentType, 3);

  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kSnapshot{
      afw::games::fc3::LastSnapshot()};
  ASSERT_EQ(kSnapshot.count, 2U);
  ASSERT_TRUE(kSnapshot.items.at(0).pawn.has_value());
  EXPECT_EQ(kSnapshot.items.at(0).pawn->army, 2);
  EXPECT_EQ(kSnapshot.items.at(0).pawn->agentType, 3U);
  EXPECT_EQ(kSnapshot.pawns, 1U);
}

// ownArmyBit == 1 << army is a real relation, but only for a pawn placed on the
// map: CBasePawnEntity_RegisterProperties attaches OnPostLoad_InitArmyMasks as
// a post-load callback, so a spawned pawn never runs it. Gating on it dropped
// every spawned NPC in the game, which is why it must not decide this.
TEST_F(Fc3SnapshotUnit, KeepsThePawnRecordWhenTheArmyBitDisagrees) {
  Poke(kAgentOne + kPawnArmy, 2);
  Poke(kAgentOne + kPawnOwnArmyBit, 1U);

  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kSnapshot{
      afw::games::fc3::LastSnapshot()};
  ASSERT_EQ(kSnapshot.count, 2U);
  ASSERT_TRUE(kSnapshot.items.at(0).pawn.has_value());
  EXPECT_EQ(kSnapshot.items.at(0).pawn->army, 2);
  EXPECT_EQ(kSnapshot.pawns, 1U);
}

// What the record exists for is the agent's class, and that is known before any
// field is read. A field that reads back nonsense costs that field, not the
// record: dropping the whole thing is how an entity ended up classified by the
// size of its bounding box instead.
TEST_F(Fc3SnapshotUnit, FallsBackOnAFieldRatherThanDroppingTheRecord) {
  Poke(kAgentOne + kPawnAgentType, 9);
  Poke(kAgentOne + kPawnArmy, 99);
  Poke(kAgentOne + kPawnFaction, 77);

  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kSnapshot{
      afw::games::fc3::LastSnapshot()};
  ASSERT_EQ(kSnapshot.count, 2U);
  ASSERT_TRUE(kSnapshot.items.at(0).pawn.has_value());

  const afw::games::fc3::PawnInfo& kPawn{*kSnapshot.items.at(0).pawn};
  EXPECT_EQ(kPawn.agentType, afw::games::fc3::kAgentUnknown);
  EXPECT_EQ(kPawn.army, afw::games::fc3::kNoArmy);
  EXPECT_EQ(kPawn.faction, afw::games::fc3::kFactionUnknown);
}

// The second entity is built without an agent at all, so it stands for
// everything in the world that is scenery: no record, and nothing downstream
// may invent a kind for it.
TEST_F(Fc3SnapshotUnit, WithholdsThePawnRecordWhenThereIsNoAgent) {
  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kSnapshot{
      afw::games::fc3::LastSnapshot()};
  ASSERT_EQ(kSnapshot.count, 2U);
  EXPECT_FALSE(kSnapshot.items.at(1).pawn.has_value());
  EXPECT_EQ(kSnapshot.pawns, 1U);
}

// The handoff itself: a walk must not change what the reader is looking at
// until the frame that publishes it. Before the triple buffer, the walk wrote
// into the very array the reader was iterating.
TEST_F(Fc3SnapshotUnit, AWalkDoesNotDisturbTheSnapshotAlreadyPublished) {
  std::ignore = Walk(At(kScheduler));
  ASSERT_EQ(afw::games::fc3::LastSnapshot().count, 2U);

  Poke(kEntityArrayOne, 0);
  afw::games::fc3::PublishCensus();
  afw::games::fc3::TakeCensus(At(kScheduler));

  EXPECT_EQ(afw::games::fc3::LastSnapshot().count, 2U);

  Settle(At(kScheduler));

  EXPECT_EQ(afw::games::fc3::LastSnapshot().count, 1U);
}

// The reader holds a reference across a whole frame, so the buffer behind it
// must not be the one the next walk writes into.
TEST_F(Fc3SnapshotUnit, TheReaderKeepsItsBufferWhileTheNextWalkRuns) {
  std::ignore = Walk(At(kScheduler));

  const afw::games::fc3::EntitySnapshot& kHeld{afw::games::fc3::LastSnapshot()};
  ASSERT_EQ(kHeld.count, 2U);

  Poke(kEntityArrayOne, 0);
  Poke(kEntityArrayTwo, 0);
  afw::games::fc3::TakeCensus(At(kScheduler));
  afw::games::fc3::TakeCensus(At(kScheduler));

  EXPECT_EQ(kHeld.count, 2U);
  EXPECT_EQ(kHeld.items.at(0).address, At(kEntityOne));
}

// ThreadSanitizer does not exist for this target, and the tests above are
// single-threaded, so nothing so far can observe the race the triple buffer
// exists to remove. This one creates it: the walk runs on its own thread while
// the reader publishes and inspects. Each entity's position is set to its own
// address, so a record stitched together from two different entities -- which
// is what the old shared-buffer handoff produced -- fails the comparison.
class Fc3HandoffUnit : public Fc3CensusUnit {
 protected:
  void SetUp() override {
    Fc3CensusUnit::SetUp();
    if (page_ == nullptr) {
      return;
    }

    PlaceEntity(kEntityOne, PositionFor(At(kEntityOne)), 0.0F, 2.0F, 5.0F);
    PlaceEntity(kEntityTwo, PositionFor(At(kEntityTwo)), 0.0F, 2.0F, 5.0F);

    Poke(kEntityArrayTwo, At(kEntityTwo));

    // The census statics outlive a test, and the fake page keeps the same base
    // address, so the dedupe table can still hold these very entities from an
    // earlier test. Closing one generation here clears it.
    Settle(At(kScheduler));
  }

  // The whole address would land far outside kMaxWorldCoordinate and be
  // rejected, so only the part that differs between the two entities is used.
  [[nodiscard]] static float PositionFor(std::uintptr_t address) {
    return static_cast<float>(address % kWorldWrap);
  }

  [[nodiscard]] static bool IsSelfConsistent(
      const afw::games::fc3::EntityInfo& entity) {
    return entity.transform.Translation().x == PositionFor(entity.address);
  }
};

// esp::DrawFrame holds the reference LastSnapshot() returns across five passes
// over the array and the whole ImGui build, which is milliseconds. So the
// invariant is not "a record is consistent when read" but "the buffer does not
// change underneath the reader for as long as it holds it".
TEST_F(Fc3HandoffUnit, TheHeldSnapshotDoesNotChangeUnderTheReader) {
  constexpr int kWalks{20000};
  constexpr int kYields{64};

  std::atomic<bool> walking{true};
  std::thread writer{[this, &walking] {
    for (int walk{0}; walk < kWalks; ++walk) {
      Poke(kEntityArrayOne, (walk % 2) == 0 ? At(kEntityOne) : At(kEntityTwo));
      afw::games::fc3::TakeCensus(At(kScheduler));
    }
    walking.store(false);
  }};

  std::uint32_t inspected{0};
  std::uint32_t torn{0};
  std::uint32_t moved{0};
  while (walking.load()) {
    afw::games::fc3::PublishCensus();

    const afw::games::fc3::EntitySnapshot& kSnapshot{
        afw::games::fc3::LastSnapshot()};
    ASSERT_LE(kSnapshot.count, afw::games::fc3::kMaxEntities);
    if (kSnapshot.count == 0) {
      continue;
    }

    const std::uint32_t kCount{kSnapshot.count};
    const std::uintptr_t kAddress{kSnapshot.items.at(0).address};
    const float kX{kSnapshot.items.at(0).transform.Translation().x};

    for (int yield{0}; yield < kYields; ++yield) {
      std::this_thread::yield();
    }

    ++inspected;
    if (kSnapshot.count != kCount ||
        kSnapshot.items.at(0).address != kAddress ||
        kSnapshot.items.at(0).transform.Translation().x != kX) {
      ++moved;
    }
    for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
      if (!IsSelfConsistent(kSnapshot.items.at(index))) {
        ++torn;
      }
    }
  }

  writer.join();

  EXPECT_EQ(moved, 0U) << "the buffer changed while the reader held it";
  EXPECT_EQ(torn, 0U);
  EXPECT_GT(inspected, 0U) << "the reader never observed a published snapshot";
}

// The component walk is what reaches a moving entity's real world matrix. The
// engine picks the component by a class hash it gets through a virtual call;
// this picks it by the data, so the test is about that choice being sound.
class Fc3ComponentUnit : public Fc3CensusUnit {
 protected:
  static constexpr std::uint32_t kEntity{0x600};
  static constexpr std::uint32_t kComponentArray{0x700};
  static constexpr std::uint32_t kComponentOne{0xA00};
  static constexpr std::uint32_t kComponentTwo{0xB80};

  static constexpr std::uint32_t kEntityComponentArray{0x88};
  static constexpr std::uint32_t kEntityComponentCount{0x8C};
  static constexpr std::uint32_t kComponentWorldMatrix{0x100};

  void SetUp() override {
    Fc3CensusUnit::SetUp();
    if (page_ == nullptr) {
      return;
    }

    Poke(kEntity + kEntityComponentArray, At(kComponentArray));
    Poke(kEntity + kEntityComponentCount, 2);

    Poke(kComponentArray, At(kComponentOne));
    Poke(kComponentArray + sizeof(std::uint32_t), At(kComponentTwo));

    PokeRigidMatrix(kComponentTwo + kComponentWorldMatrix, 7.0F, 8.0F, 9.0F);
  }

  void PokeRigidMatrix(std::uint32_t offset, float x, float y, float z) {
    for (std::uint32_t cell{0}; cell < 16; ++cell) {
      PokeFloat(offset + (cell * 4), 0.0F);
    }
    PokeFloat(offset + 0, 1.0F);
    PokeFloat(offset + 20, 1.0F);
    PokeFloat(offset + 40, 1.0F);
    PokeFloat(offset + 60, 1.0F);

    PokeFloat(offset + 48, x);
    PokeFloat(offset + 52, y);
    PokeFloat(offset + 56, z);
  }
};

TEST_F(Fc3ComponentUnit, TakesTheMatrixFromTheComponentThatHoldsABasis) {
  const auto kMatrix{afw::games::fc3::ReadComponentWorldMatrix(At(kEntity))};

  ASSERT_TRUE(kMatrix.has_value());
  EXPECT_FLOAT_EQ(kMatrix->Translation().x, 7.0F);
  EXPECT_FLOAT_EQ(kMatrix->Translation().y, 8.0F);
  EXPECT_FLOAT_EQ(kMatrix->Translation().z, 9.0F);
}

// The first component is scanned first and holds nothing basis-shaped, which is
// the normal case: an entity carries many components and only one answers.
TEST_F(Fc3ComponentUnit, StepsPastAComponentWhoseSlotIsNotABasis) {
  PokeFloat(kComponentOne + kComponentWorldMatrix, 3.0F);

  const auto kMatrix{afw::games::fc3::ReadComponentWorldMatrix(At(kEntity))};

  ASSERT_TRUE(kMatrix.has_value());
  EXPECT_FLOAT_EQ(kMatrix->Translation().x, 7.0F);
}

TEST_F(Fc3ComponentUnit, StepsOverAnEmptySlot) {
  Poke(kComponentArray, 0);

  const auto kMatrix{afw::games::fc3::ReadComponentWorldMatrix(At(kEntity))};

  ASSERT_TRUE(kMatrix.has_value());
  EXPECT_FLOAT_EQ(kMatrix->Translation().x, 7.0F);
}

TEST_F(Fc3ComponentUnit, FindsNothingWhenNoComponentHoldsABasis) {
  PokeRigidMatrix(kComponentTwo + kComponentWorldMatrix, 0.0F, 0.0F, 0.0F);
  PokeFloat(kComponentTwo + kComponentWorldMatrix, 5.0F);

  EXPECT_FALSE(
      afw::games::fc3::ReadComponentWorldMatrix(At(kEntity)).has_value());
}

TEST_F(Fc3ComponentUnit, RefusesAnImplausibleComponentCount) {
  Poke(kEntity + kEntityComponentCount, 100000);

  EXPECT_FALSE(
      afw::games::fc3::ReadComponentWorldMatrix(At(kEntity)).has_value());
}

// The skeleton's pose rides in the same component that holds the world matrix,
// and a bone is named only by a hash: the retail build keeps no bone name
// strings at all. "Head" hashes to kHeadBoneKey under the engine's own CRC-32.
class Fc3HeadBoneUnit : public Fc3ComponentUnit {
 protected:
  static constexpr std::uint32_t kBoneArray{0x2800};
  static constexpr std::uint32_t kComponentBoneArray{0x80};
  static constexpr std::uint32_t kComponentBoneCount{0x84};
  static constexpr std::uint32_t kBoneStride{0x90};
  static constexpr std::uint32_t kBonePosePosition{0x80};
  static constexpr std::uint32_t kBones{3};
  static constexpr std::uint32_t kHeadSlot{2};

  void SetUp() override {
    Fc3ComponentUnit::SetUp();
    if (page_ == nullptr) {
      return;
    }

    Poke(kComponentTwo + kComponentBoneArray, At(kBoneArray));
    Poke(kComponentTwo + kComponentBoneCount, kBones);

    PokeBone(0, 0x11111111, 0.0F, 0.0F, 0.0F);
    PokeBone(1, 0x22222222, 0.0F, 0.0F, 0.9F);
    PokeBone(kHeadSlot, afw::games::fc3::kHeadBoneKey, 0.02F, 0.0F, 1.5F);
  }

  void PokeBone(std::uint32_t slot, std::uint32_t key, float x, float y,
                float z) {
    const std::uint32_t kEntry{kBoneArray + (slot * kBoneStride)};
    Poke(kEntry, key);
    PokeFloat(kEntry + kBonePosePosition, x);
    PokeFloat(kEntry + kBonePosePosition + 4, y);
    PokeFloat(kEntry + kBonePosePosition + 8, z);
  }
};

TEST_F(Fc3HeadBoneUnit, PlacesTheHeadBoneInTheWorld) {
  const auto kHead{afw::games::fc3::ReadBonePoints(At(kEntity)).head};

  ASSERT_TRUE(kHead.has_value());
  EXPECT_FLOAT_EQ(kHead->x, 7.02F);
  EXPECT_FLOAT_EQ(kHead->y, 8.0F);
  EXPECT_FLOAT_EQ(kHead->z, 10.5F);
}

// Vehicles carry a skeleton and no head in it, so a miss is the ordinary answer
// rather than a failure.
TEST_F(Fc3HeadBoneUnit, FindsNothingWhenTheSkeletonHasNoHead) {
  PokeBone(kHeadSlot, 0x33333333, 0.0F, 0.0F, 1.5F);

  EXPECT_FALSE(afw::games::fc3::ReadBonePoints(At(kEntity)).head.has_value());
}

// A head that lands a world away came from a matrix belonging to something
// else. One stale component already put an entity 118 m from itself.
TEST_F(Fc3HeadBoneUnit, RefusesAHeadTooFarFromItsOwnBody) {
  PokeBone(kHeadSlot, afw::games::fc3::kHeadBoneKey, 0.0F, 0.0F, 40.0F);

  EXPECT_FALSE(afw::games::fc3::ReadBonePoints(At(kEntity)).head.has_value());
}

// The spine comes out of the same walk as the head, and an animal is read for
// it alone: nothing else on one is both reliably present and inside the body.
TEST_F(Fc3HeadBoneUnit, PlacesTheSpineBoneInTheWorld) {
  PokeBone(1, afw::games::fc3::kSpineBoneKey, 0.0F, 0.0F, 0.75F);

  const auto kBones{afw::games::fc3::ReadBonePoints(At(kEntity))};

  ASSERT_TRUE(kBones.body.has_value());
  EXPECT_FLOAT_EQ(kBones.body->z, 9.75F);
  EXPECT_TRUE(kBones.head.has_value());
}

// An animal has a spine and no head bone under that name, and must still come
// back with the half that exists rather than with nothing at all.
TEST_F(Fc3HeadBoneUnit, KeepsTheSpineWhenThereIsNoHead) {
  PokeBone(1, afw::games::fc3::kSpineBoneKey, 0.0F, 0.0F, 0.75F);
  PokeBone(kHeadSlot, 0x33333333, 0.0F, 0.0F, 1.5F);

  const auto kBones{afw::games::fc3::ReadBonePoints(At(kEntity))};

  EXPECT_FALSE(kBones.head.has_value());
  ASSERT_TRUE(kBones.body.has_value());
  EXPECT_FLOAT_EQ(kBones.body->z, 9.75F);
}

TEST_F(Fc3HeadBoneUnit, RefusesAnImplausibleBoneCount) {
  Poke(kComponentTwo + kComponentBoneCount, 100000);

  EXPECT_FALSE(afw::games::fc3::ReadBonePoints(At(kEntity)).head.has_value());
}

TEST_F(Fc3CensusUnit, AnImplausibleGroupCountIsCapped) {
  constexpr std::uint32_t kAbsurd{100000};
  Poke(kScheduler + kSchedulerGroupCount, kAbsurd);

  const afw::games::fc3::EntityCensus kCensus{Walk(At(kScheduler))};

  EXPECT_EQ(kCensus.skipped, kAbsurd - kMaxGroups);
}
