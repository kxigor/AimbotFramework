#include <gtest/gtest.h>

#include <afw/aimbot/aimbot.hpp>
#include <cstdint>
#include <numbers>
#include <optional>

namespace {

using afw::aimbot::AimPoint;
using afw::aimbot::Choice;
using afw::aimbot::ChooseTarget;
using afw::aimbot::Limits;
using afw::aimbot::Weights;
using afw::games::fc3::EntityInfo;
using afw::games::fc3::EntitySnapshot;
using afw::math::Vec3;

constexpr float kPi{std::numbers::pi_v<float>};
constexpr float kTolerance{1e-5F};

// The engine's world has z for up, so a direction is a yaw around z and a pitch
// away from the ground plane. Everything here is in that frame.
constexpr Vec3 kEast{.x = 1.0F, .y = 0.0F, .z = 0.0F};

[[nodiscard]] EntityInfo At(std::uintptr_t address, const Vec3& position) {
  EntityInfo entity{};
  entity.address = address;
  entity.transform.cells = {
      1.0F, 0.0F, 0.0F, 0.0F, 0.0F,       1.0F,       0.0F,       0.0F,
      0.0F, 0.0F, 1.0F, 0.0F, position.x, position.y, position.z, 1.0F};
  entity.box.min = Vec3{.x = -0.5F, .y = -0.5F, .z = 0.0F};
  entity.box.max = Vec3{.x = 0.5F, .y = 0.5F, .z = 1.8F};

  return entity;
}

[[nodiscard]] Limits Wide() {
  return Limits{.maxDistance = 200.0F, .maxError = kPi};
}

TEST(AimbotAimPoint, PrefersTheHeadWhenTheSkeletonGaveOneUp) {
  EntityInfo entity{At(1, Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F})};
  entity.head = Vec3{.x = 5.0F, .y = 0.0F, .z = 1.6F};

  EXPECT_FLOAT_EQ(AimPoint(entity).z, 1.6F);
}

TEST(AimbotAimPoint, FallsBackToTheMiddleOfTheBox) {
  const EntityInfo kEntity{At(1, Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F})};

  EXPECT_FLOAT_EQ(AimPoint(kEntity).x, 5.0F);
  EXPECT_FLOAT_EQ(AimPoint(kEntity).z, 0.9F);
}

// An animal is never given a head, so the spine is what it comes down to. A
// measured one stood 0.93 tall with its spine at 0.75 and the joints of its
// legs from 0.08 to 0.48, which puts the middle of its box under the belly.
TEST(AimbotAimPoint, AimsAtTheSpineWhenThereIsNoHead) {
  EntityInfo entity{At(1, Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F})};
  entity.body = Vec3{.x = 5.0F, .y = 0.0F, .z = 0.75F};

  EXPECT_FLOAT_EQ(AimPoint(entity).z, 0.75F);
}

TEST(AimbotAimPoint, KeepsTheHeadWhenTheSkeletonGaveUpBoth) {
  EntityInfo entity{At(1, Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F})};
  entity.head = Vec3{.x = 5.0F, .y = 0.0F, .z = 1.6F};
  entity.body = Vec3{.x = 5.0F, .y = 0.0F, .z = 1.0F};

  EXPECT_FLOAT_EQ(AimPoint(entity).z, 1.6F);
}

class AimbotChoice : public ::testing::Test {
 protected:
  void Add(std::uintptr_t address, const Vec3& position) {
    snapshot_.items.at(snapshot_.count) = At(address, position);
    ++snapshot_.count;
  }

  static std::optional<float> Anyone(const EntityInfo& /*entity*/) {
    return 0.0F;
  }

  EntitySnapshot snapshot_{};
  Vec3 eye_{.x = 0.0F, .y = 0.0F, .z = 1.6F};
};

TEST_F(AimbotChoice, FindsNobodyInAnEmptyWorld) {
  EXPECT_FALSE(
      ChooseTarget(snapshot_, eye_, kEast, Choice{.limits = Wide()}, Anyone)
          .has_value());
}

// The point of the whole thing: the winner is whoever the eye already points
// nearest to, not whoever stands closest.
TEST_F(AimbotChoice, TakesTheOneNearestTheLineOfSightNotTheNearestBody) {
  Add(0xA, Vec3{.x = 3.0F, .y = 3.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 40.0F, .y = 0.2F, .z = 0.0F});

  const auto kTarget{
      ChooseTarget(snapshot_, eye_, kEast, Choice{.limits = Wide()}, Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xBU);
}

TEST_F(AimbotChoice, ReportsWhereItWouldAimAndHowFarOff) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  snapshot_.items.at(0).head = Vec3{.x = 10.0F, .y = 0.0F, .z = 1.6F};

  const auto kTarget{
      ChooseTarget(snapshot_, eye_, kEast, Choice{.limits = Wide()}, Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_NEAR(kTarget->error, 0.0F, kTolerance);
  EXPECT_NEAR(kTarget->distance, 10.0F, kTolerance);
  EXPECT_FLOAT_EQ(kTarget->point.z, 1.6F);
}

TEST_F(AimbotChoice, LeavesAlonePeopleBeyondTheRange) {
  Add(0xA, Vec3{.x = 500.0F, .y = 0.0F, .z = 0.0F});

  const Limits kNear{.maxDistance = 100.0F, .maxError = kPi};

  EXPECT_FALSE(
      ChooseTarget(snapshot_, eye_, kEast, Choice{.limits = kNear}, Anyone)
          .has_value());
}

// Somebody standing behind the player is not a shot being missed by a little.
TEST_F(AimbotChoice, LeavesAlonePeopleOutsideTheCone) {
  Add(0xA, Vec3{.x = -10.0F, .y = 0.0F, .z = 0.0F});

  const Limits kNarrow{.maxDistance = 200.0F, .maxError = 0.2F};

  EXPECT_FALSE(
      ChooseTarget(snapshot_, eye_, kEast, Choice{.limits = kNarrow}, Anyone)
          .has_value());
}

TEST_F(AimbotChoice, AsksTheCallerWhoCounts) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 12.0F, .y = 1.0F, .z = 0.0F});

  const auto kTarget{
      ChooseTarget(snapshot_, eye_, kEast, Choice{.limits = Wide()},
                   [](const EntityInfo& entity) {
                     return entity.address == 0xBU ? std::optional<float>{0.0F}
                                                   : std::nullopt;
                   })};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xBU);
}

// The same two candidates, decided three different ways. The one off to the
// side is nearer the body, the one down the line of sight is nearer the aim,
// and the caller can say which of those it cares about -- or weigh a kind above
// both, which is what the priority term is for.
TEST_F(AimbotChoice, RanksByDistanceWhenThatIsWhatCarriesWeight) {
  Add(0xA, Vec3{.x = 3.0F, .y = 3.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 40.0F, .y = 0.2F, .z = 0.0F});

  const Weights kByRange{.aim = 0.0F, .distance = 1.0F, .priority = 0.0F};
  const auto kTarget{ChooseTarget(snapshot_, eye_, kEast,
                                  Choice{.limits = Wide(), .weights = kByRange},
                                  Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xAU);
}

TEST_F(AimbotChoice, PriorityOutweighsAimWhenItIsWorthMore) {
  Add(0xA, Vec3{.x = 3.0F, .y = 3.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 40.0F, .y = 0.2F, .z = 0.0F});

  const Weights kMixed{.aim = 1.0F, .distance = 0.0F, .priority = 1.0F};
  const auto kTarget{ChooseTarget(
      snapshot_, eye_, kEast, Choice{.limits = Wide(), .weights = kMixed},
      [](const EntityInfo& entity) {
        return std::optional<float>{entity.address == 0xAU ? 1.0F : 0.0F};
      })};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xAU);
}

// Every ranker above returns a bare float, which reaches Ranking only through
// C++20 parenthesized aggregate initialisation and leaves `reach` at its
// default. With `weights.reach` zero in all of them, the reach term was
// untested and the suite could not have noticed it going wrong. These two name
// the Ranking outright.
TEST_F(AimbotChoice, AShotThatCannotGetThereLosesToOneThatCan) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 30.0F, .y = 6.0F, .z = 0.0F});

  const Weights kByReach{
      .aim = 0.2F, .distance = 0.0F, .priority = 0.0F, .reach = 1.0F};
  const auto kTarget{ChooseTarget(
      snapshot_, eye_, kEast, Choice{.limits = Wide(), .weights = kByReach},
      [](const EntityInfo& entity) {
        return std::optional<afw::aimbot::Ranking>{afw::aimbot::Ranking{
            .priority = 0.0F, .reach = entity.address == 0xAU ? 0.0F : 1.0F}};
      })};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xBU);
  EXPECT_FLOAT_EQ(kTarget->reach, 1.0F);
}

// And the weight has to be what decides it: with reach unweighted the same two
// candidates come out the other way round.
TEST_F(AimbotChoice, ReachIsIgnoredWhenItCarriesNoWeight) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 30.0F, .y = 6.0F, .z = 0.0F});

  const Weights kByAim{
      .aim = 0.2F, .distance = 0.0F, .priority = 0.0F, .reach = 0.0F};
  const auto kTarget{ChooseTarget(
      snapshot_, eye_, kEast, Choice{.limits = Wide(), .weights = kByAim},
      [](const EntityInfo& entity) {
        return std::optional<afw::aimbot::Ranking>{afw::aimbot::Ranking{
            .priority = 0.0F, .reach = entity.address == 0xAU ? 0.0F : 1.0F}};
      })};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xAU);
}

// Weighing nothing at all must still answer, rather than tie itself into
// picking nobody out of a world that has candidates in it.
TEST_F(AimbotChoice, StillAnswersWhenEveryWeightIsZero) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});

  const Weights kNothing{.aim = 0.0F, .distance = 0.0F, .priority = 0.0F};

  EXPECT_TRUE(ChooseTarget(snapshot_, eye_, kEast,
                           Choice{.limits = Wide(), .weights = kNothing},
                           Anyone)
                  .has_value());
}

}  // namespace
