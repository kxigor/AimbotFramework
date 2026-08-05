#include <gtest/gtest.h>

#include <afw/math/matrix4.hpp>
#include <afw/math/projection.hpp>
#include <afw/math/screen.hpp>
#include <numbers>
#include <optional>

namespace {

constexpr float kWidth{1920.0F};
constexpr float kHeight{1080.0F};
constexpr float kAspect{kWidth / kHeight};
constexpr float kFov{std::numbers::pi_v<float> / 2.0F};
constexpr float kNear{0.1F};
constexpr float kFar{10000.0F};
constexpr float kCentreX{kWidth / 2.0F};
constexpr float kCentreY{kHeight / 2.0F};
constexpr float kTolerance{0.01F};

// Right-handed naming, left-handed maths: +Z ahead, +X right, +Y up, which is
// what a Direct3D title hands us.
constexpr afw::math::Vec3 kOrigin{.x = 0.0F, .y = 0.0F, .z = 0.0F};
constexpr afw::math::Vec3 kForward{.x = 0.0F, .y = 0.0F, .z = 1.0F};
constexpr afw::math::Vec3 kUp{.x = 0.0F, .y = 1.0F, .z = 0.0F};
constexpr afw::math::Vec3 kRight{.x = 1.0F, .y = 0.0F, .z = 0.0F};

[[nodiscard]] afw::math::Matrix4 DefaultViewProjection(
    const afw::math::Vec3& eye = kOrigin) {
  return afw::math::Multiply(
      afw::math::BuildView(eye, kForward, kUp, kRight),
      afw::math::BuildProjection(kFov, kAspect, kNear, kFar));
}

[[nodiscard]] std::optional<afw::math::ScreenPoint> Project(
    const afw::math::Vec3& world, const afw::math::Vec3& eye = kOrigin) {
  return afw::math::WorldToScreen(DefaultViewProjection(eye), world, kWidth,
                                  kHeight);
}

}  // namespace

TEST(MathProjectionUnit, APointStraightAheadLandsAtTheCentre) {
  const std::optional<afw::math::ScreenPoint> kPoint{
      Project({.x = 0.0F, .y = 0.0F, .z = 10.0F})};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(MathProjectionUnit, APointBehindTheCameraIsRejected) {
  EXPECT_FALSE(Project({.x = 0.0F, .y = 0.0F, .z = -10.0F}).has_value());
}

TEST(MathProjectionUnit, APointInThePlaneOfTheEyeIsRejected) {
  EXPECT_FALSE(Project({.x = 5.0F, .y = 5.0F, .z = 0.0F}).has_value());
}

TEST(MathProjectionUnit, MovingRightMovesTheScreenPointRight) {
  const std::optional<afw::math::ScreenPoint> kPoint{
      Project({.x = 1.0F, .y = 0.0F, .z = 10.0F})};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_GT(kPoint->x, kCentreX);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(MathProjectionUnit, MovingUpMovesTheScreenPointUp) {
  const std::optional<afw::math::ScreenPoint> kPoint{
      Project({.x = 0.0F, .y = 1.0F, .z = 10.0F})};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_LT(kPoint->y, kCentreY);
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
}

TEST(MathProjectionUnit, TheVerticalFieldOfViewEdgeLandsOnTheScreenEdge) {
  const std::optional<afw::math::ScreenPoint> kTop{
      Project({.x = 0.0F, .y = 10.0F, .z = 10.0F})};

  ASSERT_TRUE(kTop.has_value());
  EXPECT_NEAR(kTop->y, 0.0F, kTolerance);
}

TEST(MathProjectionUnit, TheHorizontalEdgeFollowsTheAspectRatio) {
  const std::optional<afw::math::ScreenPoint> kEdge{
      Project({.x = 10.0F * kAspect, .y = 0.0F, .z = 10.0F})};

  ASSERT_TRUE(kEdge.has_value());
  EXPECT_NEAR(kEdge->x, kWidth, kTolerance);
}

TEST(MathProjectionUnit, NearAndFarDoNotMoveTheScreenPoint) {
  const afw::math::Vec3 kWorld{.x = 3.0F, .y = -2.0F, .z = 20.0F};
  const afw::math::Matrix4 kView{
      afw::math::BuildView(kOrigin, kForward, kUp, kRight)};

  const std::optional<afw::math::ScreenPoint> kTight{afw::math::WorldToScreen(
      afw::math::Multiply(
          kView, afw::math::BuildProjection(kFov, kAspect, 0.01F, 100.0F)),
      kWorld, kWidth, kHeight)};
  const std::optional<afw::math::ScreenPoint> kWide{afw::math::WorldToScreen(
      afw::math::Multiply(
          kView, afw::math::BuildProjection(kFov, kAspect, 5.0F, 90000.0F)),
      kWorld, kWidth, kHeight)};

  ASSERT_TRUE(kTight.has_value());
  ASSERT_TRUE(kWide.has_value());
  EXPECT_NEAR(kTight->x, kWide->x, kTolerance);
  EXPECT_NEAR(kTight->y, kWide->y, kTolerance);
}

TEST(MathProjectionUnit, TheEyePositionShiftsTheWorld) {
  const afw::math::Vec3 kEye{.x = 0.0F, .y = 0.0F, .z = 5.0F};

  const std::optional<afw::math::ScreenPoint> kPoint{
      Project({.x = 0.0F, .y = 0.0F, .z = 15.0F}, kEye)};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(MathProjectionUnit, TheBasisIsUsedRatherThanTheWorldAxes) {
  const afw::math::Vec3 kLookRight{.x = 1.0F, .y = 0.0F, .z = 0.0F};
  const afw::math::Vec3 kRightOfThat{.x = 0.0F, .y = 0.0F, .z = -1.0F};

  const std::optional<afw::math::ScreenPoint> kPoint{afw::math::WorldToScreen(
      afw::math::Multiply(
          afw::math::BuildView(kOrigin, kLookRight, kUp, kRightOfThat),
          afw::math::BuildProjection(kFov, kAspect, kNear, kFar)),
      {.x = 10.0F, .y = 0.0F, .z = 0.0F}, kWidth, kHeight)};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(MathProjectionUnit, MultiplyMatchesAHandComputedProduct) {
  const afw::math::Matrix4 kLhs{.cells = {1.0F, 2.0F, 0.0F, 0.0F,  //
                                          0.0F, 1.0F, 0.0F, 0.0F,  //
                                          0.0F, 0.0F, 1.0F, 0.0F,  //
                                          3.0F, 0.0F, 0.0F, 1.0F}};
  const afw::math::Matrix4 kRhs{.cells = {2.0F, 0.0F, 0.0F, 0.0F,  //
                                          0.0F, 2.0F, 0.0F, 0.0F,  //
                                          0.0F, 0.0F, 2.0F, 0.0F,  //
                                          0.0F, 5.0F, 0.0F, 1.0F}};

  const afw::math::Matrix4 kProduct{afw::math::Multiply(kLhs, kRhs)};

  EXPECT_NEAR(kProduct.cells.at(0), 2.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(1), 4.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(5), 2.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(12), 6.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(13), 5.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(15), 1.0F, kTolerance);
}

TEST(MathProjectionUnit, TheOrderOfMultiplicationMatters) {
  const afw::math::Matrix4 kView{afw::math::BuildView(
      {.x = 1.0F, .y = 2.0F, .z = 3.0F}, kForward, kUp, kRight)};
  const afw::math::Matrix4 kProjection{
      afw::math::BuildProjection(kFov, kAspect, kNear, kFar)};

  const afw::math::Matrix4 kCorrect{afw::math::Multiply(kView, kProjection)};
  const afw::math::Matrix4 kSwapped{afw::math::Multiply(kProjection, kView)};

  EXPECT_NE(kCorrect.cells, kSwapped.cells);
}
