#include <gtest/gtest.h>

#include <afw/math/scale.hpp>

namespace {

using afw::math::Nearness;

// The score adds an angle in radians to a distance in metres, which only means
// anything once both are on the same scale. Nearness is what puts them there:
// it turns "how far off, in whatever unit" into "how good, out of one".
TEST(MathNearness, IsOneWhenNothingIsOff) {
  EXPECT_FLOAT_EQ(Nearness(0.0F, 10.0F), 1.0F);
}

TEST(MathNearness, IsZeroAtTheWorstStillAllowed) {
  EXPECT_FLOAT_EQ(Nearness(10.0F, 10.0F), 0.0F);
}

TEST(MathNearness, FallsOffInProportion) {
  EXPECT_FLOAT_EQ(Nearness(2.5F, 10.0F), 0.75F);
  EXPECT_FLOAT_EQ(Nearness(5.0F, 10.0F), 0.5F);
}

// A limit of zero admits nothing, so there is no scale to speak of. Answering
// zero keeps the term out of the sum instead of dividing by it.
TEST(MathNearness, AnswersZeroWhenThereIsNoRoom) {
  EXPECT_FLOAT_EQ(Nearness(0.0F, 0.0F), 0.0F);
  EXPECT_FLOAT_EQ(Nearness(1.0F, 0.0F), 0.0F);
  EXPECT_FLOAT_EQ(Nearness(1.0F, -10.0F), 0.0F);
}

// Callers are expected to have rejected these already -- the aimbot drops a
// candidate before scoring it -- but the arithmetic still has to be sane.
TEST(MathNearness, GoesNegativePastTheWorst) {
  EXPECT_FLOAT_EQ(Nearness(20.0F, 10.0F), -1.0F);
}

static_assert(Nearness(0.0F, 4.0F) == 1.0F);
static_assert(Nearness(4.0F, 4.0F) == 0.0F);
static_assert(Nearness(1.0F, 0.0F) == 0.0F);

}  // namespace
