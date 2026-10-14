#include <gtest/gtest.h>

#include <afw/math/angle.hpp>
#include <afw/math/direction.hpp>
#include <afw/math/vec3.hpp>
#include <cmath>
#include <limits>
#include <numbers>

namespace {

using afw::math::AngleBetween;
using afw::math::AnglesOf;
using afw::math::AnglesTo;
using afw::math::Difference;
using afw::math::Share;
using afw::math::Vec3;
using afw::math::ViewAngles;
using afw::math::WrapAngle;

constexpr float kPi{std::numbers::pi_v<float>};
constexpr float kQuarter{kPi / 2.0F};
constexpr float kTolerance{1e-5F};

// The engine's world has z for up, so a direction is a yaw around z and a pitch
// away from the ground plane. Everything here is in that frame.
constexpr Vec3 kEast{.x = 1.0F, .y = 0.0F, .z = 0.0F};
constexpr Vec3 kNorth{.x = 0.0F, .y = 1.0F, .z = 0.0F};
constexpr Vec3 kUp{.x = 0.0F, .y = 0.0F, .z = 1.0F};

TEST(MathAngles, WrapsOntoTheShortWayRound) {
  EXPECT_NEAR(WrapAngle(0.0F), 0.0F, kTolerance);
  EXPECT_NEAR(WrapAngle(kPi * 2.0F), 0.0F, kTolerance);
  EXPECT_NEAR(WrapAngle((kPi * 2.0F) + 0.25F), 0.25F, kTolerance);
  EXPECT_NEAR(WrapAngle((-kPi * 2.0F) - 0.25F), -0.25F, kTolerance);
  EXPECT_NEAR(WrapAngle((kPi * 4.0F) + 1.0F), 1.0F, kTolerance);
}

// Half a turn is the seam, and which side of it a float lands on is decided by
// the last bit. Only the magnitude is a fact worth asserting there.
TEST(MathAngles, LandsOnHalfATurnFromEitherSide) {
  EXPECT_NEAR(std::fabs(WrapAngle(kPi * 3.0F)), kPi, kTolerance);
  EXPECT_NEAR(std::fabs(WrapAngle(-kPi * 3.0F)), kPi, kTolerance);
}

// Garbage in an angle would otherwise poison every later comparison, and a
// wrapped NaN is still NaN.
TEST(MathAngles, RefusesToPropagateSomethingThatIsNotANumber) {
  EXPECT_FLOAT_EQ(WrapAngle(std::numeric_limits<float>::quiet_NaN()), 0.0F);
  EXPECT_FLOAT_EQ(WrapAngle(std::numeric_limits<float>::infinity()), 0.0F);
}

TEST(MathAngles, ConvertsBetweenDegreesAndRadians) {
  EXPECT_NEAR(afw::math::Radians(180.0F), kPi, kTolerance);
  EXPECT_NEAR(afw::math::Radians(90.0F), kQuarter, kTolerance);
  EXPECT_NEAR(afw::math::Degrees(kPi), 180.0F, kTolerance);
}

TEST(MathAngles, ReadsYawAndPitchOffADirection) {
  EXPECT_NEAR(AnglesOf(kEast).yaw, 0.0F, kTolerance);
  EXPECT_NEAR(AnglesOf(kNorth).yaw, kQuarter, kTolerance);
  EXPECT_NEAR(AnglesOf(kEast).pitch, 0.0F, kTolerance);
}

// Straight up has no yaw to speak of: every heading is equally true. Answering
// with a NaN from atan2(0, 0) instead would be the wrong kind of honest.
TEST(MathAngles, HandlesStraightUpAndStraightDown) {
  EXPECT_NEAR(AnglesOf(kUp).pitch, kQuarter, kTolerance);
  EXPECT_NEAR(AnglesOf(kUp * -1.0F).pitch, -kQuarter, kTolerance);
  EXPECT_NEAR(AnglesOf(kUp).yaw, 0.0F, kTolerance);
}

TEST(MathAngles, AimsFromOnePointToAnother) {
  const ViewAngles kAngles{AnglesTo(Vec3{.x = 10.0F, .y = 10.0F, .z = 0.0F},
                                    Vec3{.x = 10.0F, .y = 20.0F, .z = 0.0F})};

  EXPECT_NEAR(kAngles.yaw, kQuarter, kTolerance);
  EXPECT_NEAR(kAngles.pitch, 0.0F, kTolerance);
}

// The whole reason the difference is wrapped: due north to just past due south
// is a nudge one way, not most of a turn the other.
TEST(MathAngles, TakesTheShorterTurnAcrossTheSeam) {
  const ViewAngles kFrom{.yaw = kPi - 0.1F, .pitch = 0.0F};
  const ViewAngles kTo{.yaw = -kPi + 0.1F, .pitch = 0.0F};

  EXPECT_NEAR(Difference(kFrom, kTo).yaw, 0.2F, kTolerance);
  EXPECT_NEAR(Difference(kTo, kFrom).yaw, -0.2F, kTolerance);
}

TEST(MathAngles, ScalesACorrectionAndClampsTheShare) {
  const ViewAngles kCorrection{.yaw = 0.4F, .pitch = -0.2F};

  EXPECT_NEAR(Share(kCorrection, 0.5F).yaw, 0.2F, kTolerance);
  EXPECT_NEAR(Share(kCorrection, 0.5F).pitch, -0.1F, kTolerance);
  EXPECT_NEAR(Share(kCorrection, 5.0F).yaw, 0.4F, kTolerance);
  EXPECT_NEAR(Share(kCorrection, -1.0F).yaw, 0.0F, kTolerance);
}

TEST(MathAngles, MeasuresTheAngleBetweenTwoDirections) {
  EXPECT_NEAR(AngleBetween(kEast, kEast).radians, 0.0F, kTolerance);
  EXPECT_NEAR(AngleBetween(kEast, kNorth).radians, kQuarter, kTolerance);
  EXPECT_NEAR(AngleBetween(kEast, kEast * -1.0F).radians, kPi, kTolerance);
}

// A zero-length direction is the one input acos cannot be rescued from, and it
// must not read as "dead ahead".
TEST(MathAngles, TreatsANowhereDirectionAsTheWorstPossibleMatch) {
  EXPECT_NEAR(AngleBetween(kEast, Vec3{}).radians, kPi, kTolerance);
}

}  // namespace
