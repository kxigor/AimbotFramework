#include <gtest/gtest.h>

#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <cstdint>
#include <support/fc3_world.hpp>
#include <tuple>

namespace {

using afw::testing::Fc3World;

class Fc3CensusUnit : public Fc3World {};

TEST_F(Fc3CensusUnit, CountsGroupsAndTheirEntities) {
  const afw::games::fc3::EntityCensus kCensus{
      Walk(At(afw::testing::kScheduler))};

  EXPECT_EQ(kCensus.groups, 2U);
  EXPECT_EQ(kCensus.entities,
            afw::testing::kEntitiesOne + afw::testing::kEntitiesTwo);
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
  afw::games::fc3::TakeCensus(At(afw::testing::kScheduler));
  afw::games::fc3::TakeCensus(At(afw::testing::kScheduler));
  Settle(At(afw::testing::kScheduler));

  const afw::games::fc3::EntityCensus kCensus{afw::games::fc3::LastCensus()};

  EXPECT_EQ(kCensus.calls, 2U);
  EXPECT_EQ(kCensus.groups, 4U);
  EXPECT_EQ(kCensus.entities,
            2 * (afw::testing::kEntitiesOne + afw::testing::kEntitiesTwo));
}

// A frame the engine did not walk keeps the last complete world rather than
// blanking: the reader owns its buffer until a new generation is ready.
TEST_F(Fc3CensusUnit, AFrameWithoutAWalkKeepsTheLastWorld) {
  std::ignore = Walk(At(afw::testing::kScheduler));

  afw::games::fc3::PublishCensus();

  const afw::games::fc3::EntityCensus kCensus{afw::games::fc3::LastCensus()};

  EXPECT_EQ(kCensus.entities,
            afw::testing::kEntitiesOne + afw::testing::kEntitiesTwo);
}

TEST_F(Fc3CensusUnit, AnUnreadableSchedulerCountsNothing) {
  const afw::games::fc3::EntityCensus kCensus{Walk(0)};

  EXPECT_EQ(kCensus.groups, 0U);
  EXPECT_EQ(kCensus.entities, 0U);
}

TEST_F(Fc3CensusUnit, AGroupWithoutAnEntityGroupIsSkipped) {
  Poke(afw::testing::kGroupTwo + afw::games::fc3::kGroupEntityGroup, 0);

  const afw::games::fc3::EntityCensus kCensus{
      Walk(At(afw::testing::kScheduler))};

  EXPECT_EQ(kCensus.groups, 1U);
  EXPECT_EQ(kCensus.entities, afw::testing::kEntitiesOne);
}

TEST_F(Fc3CensusUnit, AnEmptyGroupIsSkipped) {
  Poke(afw::testing::kEntityGroupTwo + afw::games::fc3::kEntityGroupCount, 0);

  const afw::games::fc3::EntityCensus kCensus{
      Walk(At(afw::testing::kScheduler))};

  EXPECT_EQ(kCensus.groups, 1U);
  EXPECT_EQ(kCensus.entities, afw::testing::kEntitiesOne);
}

// Capped rather than refused: a scheduler with a wrong count still holds real
// groups at the front of its array.
TEST_F(Fc3CensusUnit, AnImplausibleGroupCountIsCapped) {
  constexpr std::uint32_t kAbsurd{100000};
  Poke(afw::testing::kScheduler + afw::games::fc3::kSchedulerGroupCount,
       kAbsurd);

  const afw::games::fc3::EntityCensus kCensus{
      Walk(At(afw::testing::kScheduler))};

  EXPECT_EQ(kCensus.skipped, kAbsurd - afw::games::fc3::kMaxGroups);
}

}  // namespace
