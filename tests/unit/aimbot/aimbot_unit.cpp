#include <gtest/gtest.h>

#include <afw/aimbot/aimbot.hpp>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>

namespace {

using afw::aimbot::AimPoint;
using afw::aimbot::AngleBetween;
using afw::aimbot::AnglesOf;
using afw::aimbot::AnglesTo;
using afw::aimbot::ChooseTarget;
using afw::aimbot::Difference;
using afw::aimbot::Limits;
using afw::aimbot::Share;
using afw::aimbot::ViewAngles;
using afw::aimbot::WrapAngle;
using afw::games::fc3::EntityInfo;
using afw::games::fc3::EntitySnapshot;
using afw::math::Vec3;

constexpr float kPi{std::numbers::pi_v<float>};
constexpr float kQuarter{kPi / 2.0F};
constexpr float kTolerance{1e-5F};

// The engine's world has z for up, so a direction is a yaw around z and a pitch
// away from the ground plane. Everything here is in that frame.
constexpr Vec3 kEast{.x = 1.0F, .y = 0.0F, .z = 0.0F};
constexpr Vec3 kNorth{.x = 0.0F, .y = 1.0F, .z = 0.0F};
constexpr Vec3 kUp{.x = 0.0F, .y = 0.0F, .z = 1.0F};

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

TEST(AimbotAngles, WrapsOntoTheShortWayRound) {
  EXPECT_NEAR(WrapAngle(0.0F), 0.0F, kTolerance);
  EXPECT_NEAR(WrapAngle(kPi * 2.0F), 0.0F, kTolerance);
  EXPECT_NEAR(WrapAngle((kPi * 2.0F) + 0.25F), 0.25F, kTolerance);
  EXPECT_NEAR(WrapAngle((-kPi * 2.0F) - 0.25F), -0.25F, kTolerance);
  EXPECT_NEAR(WrapAngle((kPi * 4.0F) + 1.0F), 1.0F, kTolerance);
}

// Half a turn is the seam, and which side of it a float lands on is decided by
// the last bit. Only the magnitude is a fact worth asserting there.
TEST(AimbotAngles, LandsOnHalfATurnFromEitherSide) {
  EXPECT_NEAR(std::fabs(WrapAngle(kPi * 3.0F)), kPi, kTolerance);
  EXPECT_NEAR(std::fabs(WrapAngle(-kPi * 3.0F)), kPi, kTolerance);
}

// Garbage in an angle would otherwise poison every later comparison, and a
// wrapped NaN is still NaN.
TEST(AimbotAngles, RefusesToPropagateSomethingThatIsNotANumber) {
  EXPECT_FLOAT_EQ(WrapAngle(std::numeric_limits<float>::quiet_NaN()), 0.0F);
  EXPECT_FLOAT_EQ(WrapAngle(std::numeric_limits<float>::infinity()), 0.0F);
}

TEST(AimbotAngles, ReadsYawAndPitchOffADirection) {
  EXPECT_NEAR(AnglesOf(kEast).yaw, 0.0F, kTolerance);
  EXPECT_NEAR(AnglesOf(kNorth).yaw, kQuarter, kTolerance);
  EXPECT_NEAR(AnglesOf(kEast).pitch, 0.0F, kTolerance);
}

// Straight up has no yaw to speak of: every heading is equally true. Answering
// with a NaN from atan2(0, 0) instead would be the wrong kind of honest.
TEST(AimbotAngles, HandlesStraightUpAndStraightDown) {
  EXPECT_NEAR(AnglesOf(kUp).pitch, kQuarter, kTolerance);
  EXPECT_NEAR(AnglesOf(kUp * -1.0F).pitch, -kQuarter, kTolerance);
  EXPECT_NEAR(AnglesOf(kUp).yaw, 0.0F, kTolerance);
}

TEST(AimbotAngles, AimsFromOnePointToAnother) {
  const ViewAngles kAngles{AnglesTo(Vec3{.x = 10.0F, .y = 10.0F, .z = 0.0F},
                                    Vec3{.x = 10.0F, .y = 20.0F, .z = 0.0F})};

  EXPECT_NEAR(kAngles.yaw, kQuarter, kTolerance);
  EXPECT_NEAR(kAngles.pitch, 0.0F, kTolerance);
}

// The whole reason the difference is wrapped: due north to just past due south
// is a nudge one way, not most of a turn the other.
TEST(AimbotAngles, TakesTheShorterTurnAcrossTheSeam) {
  const ViewAngles kFrom{.yaw = kPi - 0.1F, .pitch = 0.0F};
  const ViewAngles kTo{.yaw = -kPi + 0.1F, .pitch = 0.0F};

  EXPECT_NEAR(Difference(kFrom, kTo).yaw, 0.2F, kTolerance);
  EXPECT_NEAR(Difference(kTo, kFrom).yaw, -0.2F, kTolerance);
}

TEST(AimbotAngles, ScalesACorrectionAndClampsTheShare) {
  const ViewAngles kCorrection{.yaw = 0.4F, .pitch = -0.2F};

  EXPECT_NEAR(Share(kCorrection, 0.5F).yaw, 0.2F, kTolerance);
  EXPECT_NEAR(Share(kCorrection, 0.5F).pitch, -0.1F, kTolerance);
  EXPECT_NEAR(Share(kCorrection, 5.0F).yaw, 0.4F, kTolerance);
  EXPECT_NEAR(Share(kCorrection, -1.0F).yaw, 0.0F, kTolerance);
}

TEST(AimbotAngles, MeasuresTheAngleBetweenTwoDirections) {
  EXPECT_NEAR(AngleBetween(kEast, kEast), 0.0F, kTolerance);
  EXPECT_NEAR(AngleBetween(kEast, kNorth), kQuarter, kTolerance);
  EXPECT_NEAR(AngleBetween(kEast, kEast * -1.0F), kPi, kTolerance);
}

// A zero-length direction is the one input acos cannot be rescued from, and it
// must not read as "dead ahead".
TEST(AimbotAngles, TreatsANowhereDirectionAsTheWorstPossibleMatch) {
  EXPECT_NEAR(AngleBetween(kEast, Vec3{}), kPi, kTolerance);
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

TEST(AimbotLead, PutsTheAimWhereTheTargetWillBe) {
  const Vec3 kPoint{.x = 10.0F, .y = 0.0F, .z = 1.6F};
  const Vec3 kWalking{.x = 0.0F, .y = 4.0F, .z = 0.0F};

  EXPECT_FLOAT_EQ(afw::aimbot::Lead(kPoint, kWalking, 0.5F).y, 2.0F);
}

// A standing target, a shot that takes no time, or a velocity that is not a
// number must all leave the aim exactly where it was.
TEST(AimbotLead, LeavesTheAimAloneWhenThereIsNothingToLead) {
  const Vec3 kPoint{.x = 10.0F, .y = 0.0F, .z = 1.6F};

  EXPECT_FLOAT_EQ(afw::aimbot::Lead(kPoint, Vec3{}, 0.5F).y, 0.0F);
  EXPECT_FLOAT_EQ(afw::aimbot::Lead(kPoint, Vec3{.x = 0.0F, .y = 4.0F}, 0.0F).y,
                  0.0F);
  EXPECT_FLOAT_EQ(afw::aimbot::Lead(kPoint, Vec3{.x = 0.0F, .y = 4.0F},
                                    std::numeric_limits<float>::quiet_NaN())
                      .y,
                  0.0F);
}

TEST(AimbotLead, TurnsDistanceIntoFlightTime) {
  EXPECT_FLOAT_EQ(afw::aimbot::TravelTime(100.0F, 250.0F), 0.4F);
  EXPECT_FLOAT_EQ(afw::aimbot::TravelTime(100.0F, 0.0F), 0.0F);
  EXPECT_FLOAT_EQ(afw::aimbot::TravelTime(-1.0F, 250.0F), 0.0F);
}

// The first sighting has nothing to difference against, so it must report no
// movement rather than a jump from the origin.
TEST(AimbotLead, WaitsForASecondSightingBeforeBelievingAVelocity) {
  afw::aimbot::MotionTable table{};
  table.Observe(0xA, Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}, 0.1F);

  EXPECT_FLOAT_EQ(table.VelocityOf(0xA).y, 0.0F);

  table.Observe(0xA, Vec3{.x = 0.0F, .y = 0.4F, .z = 0.0F}, 0.1F);

  EXPECT_NEAR(table.VelocityOf(0xA).y, 4.0F, kTolerance);
  EXPECT_FLOAT_EQ(table.VelocityOf(0xB).y, 0.0F);
}

// Killing a target sends it to ragdoll and its head bone travels further in one
// frame than anybody walks in a second. Believing that threw the aim right off
// the target it had just killed, which is the bug this guards.
TEST(AimbotLead, RefusesAStepNothingCouldHaveWalked) {
  afw::aimbot::MotionTable table{};
  table.Observe(0xA, Vec3{.x = 0.0F, .y = 0.0F, .z = 1.6F}, 0.016F);
  table.Observe(0xA, Vec3{.x = 0.0F, .y = 0.4F, .z = 1.6F}, 0.016F);

  ASSERT_GT(table.VelocityOf(0xA).y, 0.0F);

  table.Observe(0xA, Vec3{.x = 0.0F, .y = 40.0F, .z = 0.2F}, 0.016F);

  EXPECT_FLOAT_EQ(table.VelocityOf(0xA).y, 0.0F);
  EXPECT_FLOAT_EQ(table.VelocityOf(0xA).z, 0.0F);
}

class AimbotChoice : public ::testing::Test {
 protected:
  void Add(std::uintptr_t address, const Vec3& position) {
    snapshot_.items.at(snapshot_.count) = At(address, position);
    ++snapshot_.count;
  }

  static bool Anyone(const EntityInfo& /*entity*/) { return true; }

  EntitySnapshot snapshot_{};
  Vec3 eye_{.x = 0.0F, .y = 0.0F, .z = 1.6F};
};

TEST_F(AimbotChoice, FindsNobodyInAnEmptyWorld) {
  EXPECT_FALSE(
      ChooseTarget(snapshot_, eye_, kEast, Wide(), Anyone).has_value());
}

// The point of the whole thing: the winner is whoever the eye already points
// nearest to, not whoever stands closest.
TEST_F(AimbotChoice, TakesTheOneNearestTheLineOfSightNotTheNearestBody) {
  Add(0xA, Vec3{.x = 3.0F, .y = 3.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 40.0F, .y = 0.2F, .z = 0.0F});

  const auto kTarget{ChooseTarget(snapshot_, eye_, kEast, Wide(), Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xBU);
}

TEST_F(AimbotChoice, ReportsWhereItWouldAimAndHowFarOff) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  snapshot_.items.at(0).head = Vec3{.x = 10.0F, .y = 0.0F, .z = 1.6F};

  const auto kTarget{ChooseTarget(snapshot_, eye_, kEast, Wide(), Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_NEAR(kTarget->error, 0.0F, kTolerance);
  EXPECT_NEAR(kTarget->distance, 10.0F, kTolerance);
  EXPECT_FLOAT_EQ(kTarget->point.z, 1.6F);
}

TEST_F(AimbotChoice, LeavesAlonePeopleBeyondTheRange) {
  Add(0xA, Vec3{.x = 500.0F, .y = 0.0F, .z = 0.0F});

  const Limits kNear{.maxDistance = 100.0F, .maxError = kPi};

  EXPECT_FALSE(ChooseTarget(snapshot_, eye_, kEast, kNear, Anyone).has_value());
}

// Somebody standing behind the player is not a shot being missed by a little.
TEST_F(AimbotChoice, LeavesAlonePeopleOutsideTheCone) {
  Add(0xA, Vec3{.x = -10.0F, .y = 0.0F, .z = 0.0F});

  const Limits kNarrow{.maxDistance = 200.0F, .maxError = 0.2F};

  EXPECT_FALSE(
      ChooseTarget(snapshot_, eye_, kEast, kNarrow, Anyone).has_value());
}

TEST_F(AimbotChoice, AsksTheCallerWhoCounts) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 12.0F, .y = 1.0F, .z = 0.0F});

  const auto kTarget{ChooseTarget(
      snapshot_, eye_, kEast, Wide(),
      [](const EntityInfo& entity) { return entity.address == 0xBU; })};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xBU);
}

}  // namespace
