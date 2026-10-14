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
using afw::aimbot::RankFn;
using afw::aimbot::Ranking;
using afw::aimbot::Weights;
using afw::games::fc3::EntityInfo;
using afw::games::fc3::EntitySnapshot;
using afw::games::fc3::PawnInfo;
using afw::math::Angle;
using afw::math::Ray;
using afw::math::Vec3;

constexpr float kPi{std::numbers::pi_v<float>};
constexpr float kTolerance{1e-5F};

// The engine's world has z for up, so a direction is a yaw around z and a pitch
// away from the ground plane. Everything here is in that frame.
constexpr Vec3 kEast{.x = 1.0F, .y = 0.0F, .z = 0.0F};

// Every ranker below names both fields of Ranking, because the constructor
// leaves no way not to. That is the point of it: while Ranking was an
// aggregate, a ranker returning a bare float reached it through C++20
// parenthesized aggregate initialisation and left `reach` at a default nobody
// had chosen, so the reach term went untested without the suite noticing.
constexpr Ranking kPlainRanking{/*priority = */ 0.0F, /*reach = */ 1.0F};

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
  return Limits{/*maxDistance = */ 200.0F, /*maxError = */ Angle{kPi}};
}

[[nodiscard]] Weights ByAim() {
  return Weights{/*aim = */ 1.0F, /*distance = */ 0.0F, /*priority = */ 0.0F,
                 /*reach = */ 0.0F};
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

// The rankers stand in for what the injected build reads out of the menu and
// the entity's faction. They are free functions rather than lambdas because
// ChooseTarget now takes a plain function pointer.
[[nodiscard]] std::optional<Ranking> Anyone(const EntityInfo& /*entity*/,
                                            const PawnInfo* /*self*/) noexcept {
  return std::optional<Ranking>{kPlainRanking};
}

[[nodiscard]] std::optional<Ranking> OnlyB(const EntityInfo& entity,
                                           const PawnInfo* /*self*/) noexcept {
  if (entity.address != 0xBU) {
    return std::nullopt;
  }

  return std::optional<Ranking>{kPlainRanking};
}

[[nodiscard]] std::optional<Ranking> FavourA(
    const EntityInfo& entity, const PawnInfo* /*self*/) noexcept {
  return std::optional<Ranking>{
      Ranking{/*priority = */ entity.address == 0xAU ? 1.0F : 0.0F,
              /*reach    = */ 1.0F}};
}

[[nodiscard]] std::optional<Ranking> ReachOfB(
    const EntityInfo& entity, const PawnInfo* /*self*/) noexcept {
  return std::optional<Ranking>{
      Ranking{/*priority = */ 0.0F,
              /*reach    = */ entity.address == 0xAU ? 0.0F : 1.0F}};
}

// The score sums four terms, and the weights only mean what the menu's sliders
// suggest while all four sit on the same 0..1 scale. Two of them are put there
// by Limits::NearError and Limits::NearRange; the other two arrive from the
// ranker exactly as it wrote them, and nothing checks or repairs that. Far Cry 3
// holds the scale by accident of the user interface -- config::kMaxPriority
// bounds the slider and ReachOf answers 0 or 1 -- which is fine for one title
// and invisible to the author of the next. This ranker answers on a scale a
// hundred times wider, the way a binding with its own notion of priority might.
constexpr float kOffScalePriority{100.0F};

[[nodiscard]] std::optional<Ranking> OffScaleFavourA(
    const EntityInfo& entity, const PawnInfo* /*self*/) noexcept {
  return std::optional<Ranking>{
      Ranking{/*priority = */ entity.address == 0xAU ? kOffScalePriority : 0.0F,
              /*reach    = */ 1.0F}};
}

class AimbotChoice : public ::testing::Test {
 protected:
  void Add(std::uintptr_t address, const Vec3& position) {
    snapshot_.items.at(snapshot_.count) = At(address, position);
    ++snapshot_.count;
  }

  [[nodiscard]] Ray View() const {
    return Ray{.origin = eye_, .direction = kEast};
  }

  [[nodiscard]] std::optional<afw::aimbot::Target> Pick(const Choice& choice,
                                                        RankFn rank) const {
    return ChooseTarget(snapshot_, View(), choice, rank, nullptr);
  }

  EntitySnapshot snapshot_{};
  Vec3 eye_{.x = 0.0F, .y = 0.0F, .z = 1.6F};
};

TEST_F(AimbotChoice, FindsNobodyInAnEmptyWorld) {
  EXPECT_FALSE(Pick(Choice{Wide(), ByAim()}, Anyone).has_value());
}

// The point of the whole thing: the winner is whoever the eye already points
// nearest to, not whoever stands closest.
TEST_F(AimbotChoice, TakesTheOneNearestTheLineOfSightNotTheNearestBody) {
  Add(0xA, Vec3{.x = 3.0F, .y = 3.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 40.0F, .y = 0.2F, .z = 0.0F});

  const auto kTarget{Pick(Choice{Wide(), ByAim()}, Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xBU);
}

TEST_F(AimbotChoice, ReportsWhereItWouldAimAndHowFarOff) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  snapshot_.items.at(0).head = Vec3{.x = 10.0F, .y = 0.0F, .z = 1.6F};

  const auto kTarget{Pick(Choice{Wide(), ByAim()}, Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_NEAR(kTarget->error.radians, 0.0F, kTolerance);
  EXPECT_NEAR(kTarget->distance, 10.0F, kTolerance);
  EXPECT_FLOAT_EQ(kTarget->point.z, 1.6F);
}

TEST_F(AimbotChoice, LeavesAlonePeopleBeyondTheRange) {
  Add(0xA, Vec3{.x = 500.0F, .y = 0.0F, .z = 0.0F});

  const Limits kNear{/*maxDistance = */ 100.0F, /*maxError = */ Angle{kPi}};

  EXPECT_FALSE(Pick(Choice{kNear, ByAim()}, Anyone).has_value());
}

// Somebody standing behind the player is not a shot being missed by a little.
TEST_F(AimbotChoice, LeavesAlonePeopleOutsideTheCone) {
  Add(0xA, Vec3{.x = -10.0F, .y = 0.0F, .z = 0.0F});

  const Limits kNarrow{/*maxDistance = */ 200.0F,
                       /*maxError = */ Angle{0.2F}};

  EXPECT_FALSE(Pick(Choice{kNarrow, ByAim()}, Anyone).has_value());
}

TEST_F(AimbotChoice, AsksTheCallerWhoCounts) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 12.0F, .y = 1.0F, .z = 0.0F});

  const auto kTarget{Pick(Choice{Wide(), ByAim()}, OnlyB)};

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

  const Weights kByRange{/*aim = */ 0.0F, /*distance = */ 1.0F,
                         /*priority = */ 0.0F, /*reach = */ 0.0F};

  const auto kTarget{Pick(Choice{Wide(), kByRange}, Anyone)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xAU);
}

TEST_F(AimbotChoice, PriorityOutweighsAimWhenItIsWorthMore) {
  Add(0xA, Vec3{.x = 3.0F, .y = 3.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 40.0F, .y = 0.2F, .z = 0.0F});

  const Weights kMixed{/*aim = */ 1.0F, /*distance = */ 0.0F,
                       /*priority = */ 1.0F, /*reach = */ 0.0F};

  const auto kTarget{Pick(Choice{Wide(), kMixed}, FavourA)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xAU);
}

// Not a prohibition -- the aimbot neither checks the scale nor repairs it, and
// the pick stays a well defined maximum either way. This records what it costs,
// so the next binding's author meets the contract here rather than in the game.
TEST_F(AimbotChoice, APriorityOffTheAgreedScaleOverrulesItsOwnWeight) {
  Add(0xA, Vec3{.x = 3.0F, .y = 3.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 40.0F, .y = 0.2F, .z = 0.0F});

  // Aim is weighted a hundred times above priority, so priority can only ever
  // break a tie -- as long as the ranker answers on the scale the weights assume.
  const Weights kAimFirst{/*aim = */ 1.0F, /*distance = */ 0.0F,
                          /*priority = */ 0.01F, /*reach = */ 0.0F};

  const auto kInScale{Pick(Choice{Wide(), kAimFirst}, FavourA)};

  ASSERT_TRUE(kInScale.has_value());
  EXPECT_EQ(kInScale->entity, 0xBU);

  // Same weights, same world, a ranker that answers 100 where the other answered
  // 1: the term meant to break ties decides the whole thing instead.
  const auto kOffScale{Pick(Choice{Wide(), kAimFirst}, OffScaleFavourA)};

  ASSERT_TRUE(kOffScale.has_value());
  EXPECT_EQ(kOffScale->entity, 0xAU);
}

TEST_F(AimbotChoice, AShotThatCannotGetThereLosesToOneThatCan) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 30.0F, .y = 6.0F, .z = 0.0F});

  const Weights kByReach{/*aim = */ 0.2F, /*distance = */ 0.0F,
                         /*priority = */ 0.0F, /*reach = */ 1.0F};

  const auto kTarget{Pick(Choice{Wide(), kByReach}, ReachOfB)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xBU);
  EXPECT_FLOAT_EQ(kTarget->ranking.reach, 1.0F);
}

// And the weight has to be what decides it: with reach unweighted the same two
// candidates come out the other way round.
TEST_F(AimbotChoice, ReachIsIgnoredWhenItCarriesNoWeight) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});
  Add(0xB, Vec3{.x = 30.0F, .y = 6.0F, .z = 0.0F});

  const Weights kByAim{/*aim = */ 0.2F, /*distance = */ 0.0F,
                       /*priority = */ 0.0F, /*reach = */ 0.0F};

  const auto kTarget{Pick(Choice{Wide(), kByAim}, ReachOfB)};

  ASSERT_TRUE(kTarget.has_value());
  EXPECT_EQ(kTarget->entity, 0xAU);
}

// WantsReach() is what the injected build asks before spending a frame's worth
// of traces: dll::UpdateReach returns early when it answers no, and ReachOf then
// hands every entity the 1.0F it defaults to, so the term contributes the same
// constant to every candidate and cannot change the pick. Weights arrive from
// sliders the menu bounds below at zero, so "carries weight" and "is positive"
// are the same question here, and it is asked with an ordering comparison --
// this project never tests a float for equality.
TEST_F(AimbotChoice, AnUnweightedReachDoesNotPayForTraces) {
  const Weights kNoReach{/*aim = */ 0.2F, /*distance = */ 0.0F,
                         /*priority = */ 0.0F, /*reach = */ 0.0F};
  const Choice kUnweighted{Wide(), kNoReach};

  EXPECT_FALSE(kUnweighted.WantsReach());

  const Weights kByReach{/*aim = */ 0.2F, /*distance = */ 0.0F,
                         /*priority = */ 0.0F, /*reach = */ 1.0F};
  const Choice kWanted{Wide(), kByReach};

  EXPECT_TRUE(kWanted.WantsReach());
}

// Weighing nothing at all must still answer, rather than tie itself into
// picking nobody out of a world that has candidates in it.
TEST_F(AimbotChoice, StillAnswersWhenEveryWeightIsZero) {
  Add(0xA, Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F});

  const Weights kNothing{/*aim = */ 0.0F, /*distance = */ 0.0F,
                         /*priority = */ 0.0F, /*reach = */ 0.0F};

  EXPECT_TRUE(Pick(Choice{Wide(), kNothing}, Anyone).has_value());
}

}  // namespace
