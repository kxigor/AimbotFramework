#include <gtest/gtest.h>

#include <afw/math/vec3.hpp>
#include <array>
#include <cstddef>
#include <cstring>

namespace {

constexpr float kTolerance{1e-5F};

}  // namespace

// The layout is a contract with the game's memory, not an implementation
// detail: afw::process::ReadAt memcpies three consecutive floats into this.
TEST(Vec3Unit, LaysOutThreeFloatsInOrderWithNoPadding) {
  EXPECT_EQ(offsetof(afw::math::Vec3, x), 0U);
  EXPECT_EQ(offsetof(afw::math::Vec3, y), sizeof(float));
  EXPECT_EQ(offsetof(afw::math::Vec3, z), 2 * sizeof(float));
  EXPECT_EQ(sizeof(afw::math::Vec3), 3 * sizeof(float));
}

TEST(Vec3Unit, ReproducesThreeFloatsCopiedOverIt) {
  const std::array<float, 3> kSource{1.5F, -2.25F, 4.0F};

  afw::math::Vec3 value{};
  std::memcpy(static_cast<void*>(&value), kSource.data(), sizeof(value));

  EXPECT_FLOAT_EQ(value.x, 1.5F);
  EXPECT_FLOAT_EQ(value.y, -2.25F);
  EXPECT_FLOAT_EQ(value.z, 4.0F);
}

TEST(Vec3Unit, DotMatchesTheHandComputedSum) {
  const afw::math::Vec3 kLhs{.x = 1.0F, .y = 2.0F, .z = 3.0F};
  const afw::math::Vec3 kRhs{.x = 4.0F, .y = -5.0F, .z = 6.0F};

  EXPECT_FLOAT_EQ(kLhs.Dot(kRhs), (4.0F - 10.0F) + 18.0F);
}

TEST(Vec3Unit, CrossOfTheUnitAxesFollowsTheRightHandRule) {
  const afw::math::Vec3 kX{.x = 1.0F, .y = 0.0F, .z = 0.0F};
  const afw::math::Vec3 kY{.x = 0.0F, .y = 1.0F, .z = 0.0F};

  const afw::math::Vec3 kResult{kX.Cross(kY)};

  EXPECT_FLOAT_EQ(kResult.x, 0.0F);
  EXPECT_FLOAT_EQ(kResult.y, 0.0F);
  EXPECT_FLOAT_EQ(kResult.z, 1.0F);
}

// The sign of a cross product is what decided whether the whole overlay was
// mirrored, so it gets a test of its own.
TEST(Vec3Unit, CrossIsAntiCommutative) {
  const afw::math::Vec3 kLhs{.x = 1.0F, .y = 2.0F, .z = 3.0F};
  const afw::math::Vec3 kRhs{.x = -4.0F, .y = 5.0F, .z = 6.0F};

  const afw::math::Vec3 kForward{kLhs.Cross(kRhs)};
  const afw::math::Vec3 kReversed{kRhs.Cross(kLhs)};

  EXPECT_FLOAT_EQ(kForward.x, -kReversed.x);
  EXPECT_FLOAT_EQ(kForward.y, -kReversed.y);
  EXPECT_FLOAT_EQ(kForward.z, -kReversed.z);
}

TEST(Vec3Unit, LengthOfAThreeFourFiveTriangle) {
  const afw::math::Vec3 kValue{.x = 3.0F, .y = 4.0F, .z = 0.0F};

  EXPECT_FLOAT_EQ(kValue.Length(), 5.0F);
}

TEST(Vec3Unit, NormalisedKeepsTheDirectionAndUnitLength) {
  const afw::math::Vec3 kValue{.x = 0.0F, .y = 0.0F, .z = -7.0F};

  const afw::math::Vec3 kUnit{kValue.Normalised()};

  EXPECT_FLOAT_EQ(kUnit.z, -1.0F);
  EXPECT_TRUE(kUnit.IsUnitLength(kTolerance));
}

TEST(Vec3Unit, NormalisingAZeroVectorYieldsZeroRatherThanNaN) {
  const afw::math::Vec3 kUnit{afw::math::Vec3{}.Normalised()};

  EXPECT_FLOAT_EQ(kUnit.x, 0.0F);
  EXPECT_FLOAT_EQ(kUnit.y, 0.0F);
  EXPECT_FLOAT_EQ(kUnit.z, 0.0F);
}

TEST(Vec3Unit, IsUnitLengthAnswersOnBothSidesOfTheTolerance) {
  const afw::math::Vec3 kValue{.x = 1.02F, .y = 0.0F, .z = 0.0F};

  EXPECT_TRUE(kValue.IsUnitLength(0.05F));
  EXPECT_FALSE(kValue.IsUnitLength(0.01F));
}

TEST(Vec3Unit, SubtractionLeavesItsOperandsAlone) {
  const afw::math::Vec3 kLhs{.x = 5.0F, .y = 5.0F, .z = 5.0F};
  const afw::math::Vec3 kRhs{.x = 1.0F, .y = 2.0F, .z = 3.0F};

  const afw::math::Vec3 kDelta{kLhs - kRhs};

  EXPECT_FLOAT_EQ(kDelta.x, 4.0F);
  EXPECT_FLOAT_EQ(kDelta.y, 3.0F);
  EXPECT_FLOAT_EQ(kDelta.z, 2.0F);
  EXPECT_FLOAT_EQ(kLhs.x, 5.0F);
  EXPECT_FLOAT_EQ(kRhs.z, 3.0F);
}

TEST(Vec3Unit, AdditionAndScalingComposeAsAnOffset) {
  const afw::math::Vec3 kOrigin{.x = 1.0F, .y = 1.0F, .z = 1.0F};
  const afw::math::Vec3 kDirection{.x = 0.0F, .y = 1.0F, .z = 0.0F};

  const afw::math::Vec3 kMoved{kOrigin + (kDirection * 2.5F)};

  EXPECT_FLOAT_EQ(kMoved.x, 1.0F);
  EXPECT_FLOAT_EQ(kMoved.y, 3.5F);
  EXPECT_FLOAT_EQ(kMoved.z, 1.0F);
}
