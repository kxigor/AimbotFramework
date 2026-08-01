#include <gtest/gtest.h>

#include <afw/esp/projection.hpp>
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
constexpr afw::esp::Vec3 kOrigin{.x = 0.0F, .y = 0.0F, .z = 0.0F};
constexpr afw::esp::Vec3 kForward{.x = 0.0F, .y = 0.0F, .z = 1.0F};
constexpr afw::esp::Vec3 kUp{.x = 0.0F, .y = 1.0F, .z = 0.0F};
constexpr afw::esp::Vec3 kRight{.x = 1.0F, .y = 0.0F, .z = 0.0F};

[[nodiscard]] afw::esp::Matrix4 DefaultViewProjection(
    const afw::esp::Vec3& eye = kOrigin) {
  return afw::esp::Multiply(
      afw::esp::BuildView(eye, kForward, kUp, kRight),
      afw::esp::BuildProjection(kFov, kAspect, kNear, kFar));
}

[[nodiscard]] std::optional<afw::esp::ScreenPoint> Project(
    const afw::esp::Vec3& world, const afw::esp::Vec3& eye = kOrigin) {
  return afw::esp::WorldToScreen(DefaultViewProjection(eye), world, kWidth,
                                 kHeight);
}

}  // namespace

TEST(EspProjectionUnit, APointStraightAheadLandsAtTheCentre) {
  const std::optional<afw::esp::ScreenPoint> kPoint{
      Project({.x = 0.0F, .y = 0.0F, .z = 10.0F})};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(EspProjectionUnit, APointBehindTheCameraIsRejected) {
  EXPECT_FALSE(Project({.x = 0.0F, .y = 0.0F, .z = -10.0F}).has_value());
}

TEST(EspProjectionUnit, APointInThePlaneOfTheEyeIsRejected) {
  EXPECT_FALSE(Project({.x = 5.0F, .y = 5.0F, .z = 0.0F}).has_value());
}

TEST(EspProjectionUnit, MovingRightMovesTheScreenPointRight) {
  const std::optional<afw::esp::ScreenPoint> kPoint{
      Project({.x = 1.0F, .y = 0.0F, .z = 10.0F})};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_GT(kPoint->x, kCentreX);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(EspProjectionUnit, MovingUpMovesTheScreenPointUp) {
  const std::optional<afw::esp::ScreenPoint> kPoint{
      Project({.x = 0.0F, .y = 1.0F, .z = 10.0F})};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_LT(kPoint->y, kCentreY);
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
}

TEST(EspProjectionUnit, TheVerticalFieldOfViewEdgeLandsOnTheScreenEdge) {
  const std::optional<afw::esp::ScreenPoint> kTop{
      Project({.x = 0.0F, .y = 10.0F, .z = 10.0F})};

  ASSERT_TRUE(kTop.has_value());
  EXPECT_NEAR(kTop->y, 0.0F, kTolerance);
}

TEST(EspProjectionUnit, TheHorizontalEdgeFollowsTheAspectRatio) {
  const std::optional<afw::esp::ScreenPoint> kEdge{
      Project({.x = 10.0F * kAspect, .y = 0.0F, .z = 10.0F})};

  ASSERT_TRUE(kEdge.has_value());
  EXPECT_NEAR(kEdge->x, kWidth, kTolerance);
}

TEST(EspProjectionUnit, NearAndFarDoNotMoveTheScreenPoint) {
  const afw::esp::Vec3 kWorld{.x = 3.0F, .y = -2.0F, .z = 20.0F};
  const afw::esp::Matrix4 kView{
      afw::esp::BuildView(kOrigin, kForward, kUp, kRight)};

  const std::optional<afw::esp::ScreenPoint> kTight{afw::esp::WorldToScreen(
      afw::esp::Multiply(
          kView, afw::esp::BuildProjection(kFov, kAspect, 0.01F, 100.0F)),
      kWorld, kWidth, kHeight)};
  const std::optional<afw::esp::ScreenPoint> kWide{afw::esp::WorldToScreen(
      afw::esp::Multiply(
          kView, afw::esp::BuildProjection(kFov, kAspect, 5.0F, 90000.0F)),
      kWorld, kWidth, kHeight)};

  ASSERT_TRUE(kTight.has_value());
  ASSERT_TRUE(kWide.has_value());
  EXPECT_NEAR(kTight->x, kWide->x, kTolerance);
  EXPECT_NEAR(kTight->y, kWide->y, kTolerance);
}

TEST(EspProjectionUnit, TheEyePositionShiftsTheWorld) {
  const afw::esp::Vec3 kEye{.x = 0.0F, .y = 0.0F, .z = 5.0F};

  const std::optional<afw::esp::ScreenPoint> kPoint{
      Project({.x = 0.0F, .y = 0.0F, .z = 15.0F}, kEye)};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(EspProjectionUnit, TheBasisIsUsedRatherThanTheWorldAxes) {
  const afw::esp::Vec3 kLookRight{.x = 1.0F, .y = 0.0F, .z = 0.0F};
  const afw::esp::Vec3 kRightOfThat{.x = 0.0F, .y = 0.0F, .z = -1.0F};

  const std::optional<afw::esp::ScreenPoint> kPoint{afw::esp::WorldToScreen(
      afw::esp::Multiply(
          afw::esp::BuildView(kOrigin, kLookRight, kUp, kRightOfThat),
          afw::esp::BuildProjection(kFov, kAspect, kNear, kFar)),
      {.x = 10.0F, .y = 0.0F, .z = 0.0F}, kWidth, kHeight)};

  ASSERT_TRUE(kPoint.has_value());
  EXPECT_NEAR(kPoint->x, kCentreX, kTolerance);
  EXPECT_NEAR(kPoint->y, kCentreY, kTolerance);
}

TEST(EspProjectionUnit, MultiplyMatchesAHandComputedProduct) {
  const afw::esp::Matrix4 kLhs{.cells = {1.0F, 2.0F, 0.0F, 0.0F,  //
                                         0.0F, 1.0F, 0.0F, 0.0F,  //
                                         0.0F, 0.0F, 1.0F, 0.0F,  //
                                         3.0F, 0.0F, 0.0F, 1.0F}};
  const afw::esp::Matrix4 kRhs{.cells = {2.0F, 0.0F, 0.0F, 0.0F,  //
                                         0.0F, 2.0F, 0.0F, 0.0F,  //
                                         0.0F, 0.0F, 2.0F, 0.0F,  //
                                         0.0F, 5.0F, 0.0F, 1.0F}};

  const afw::esp::Matrix4 kProduct{afw::esp::Multiply(kLhs, kRhs)};

  EXPECT_NEAR(kProduct.cells.at(0), 2.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(1), 4.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(5), 2.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(12), 6.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(13), 5.0F, kTolerance);
  EXPECT_NEAR(kProduct.cells.at(15), 1.0F, kTolerance);
}

TEST(EspProjectionUnit, TheOrderOfMultiplicationMatters) {
  const afw::esp::Matrix4 kView{afw::esp::BuildView(
      {.x = 1.0F, .y = 2.0F, .z = 3.0F}, kForward, kUp, kRight)};
  const afw::esp::Matrix4 kProjection{
      afw::esp::BuildProjection(kFov, kAspect, kNear, kFar)};

  const afw::esp::Matrix4 kCorrect{afw::esp::Multiply(kView, kProjection)};
  const afw::esp::Matrix4 kSwapped{afw::esp::Multiply(kProjection, kView)};

  EXPECT_NE(kCorrect.cells, kSwapped.cells);
}
