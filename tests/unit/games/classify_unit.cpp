#include <gtest/gtest.h>

#include <afw/games/fc3/internal/entity.hpp>
#include <cstdint>
#include <limits>

namespace {

// Classify touches no memory, so every one of these runs without the game.
[[nodiscard]] afw::math::Matrix4 RigidAt(float x, float y, float z) {
  afw::math::Matrix4 matrix{};
  matrix.cells = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                  0.0F, 0.0F, 1.0F, 0.0F, x,    y,    z,    1.0F};

  return matrix;
}

[[nodiscard]] afw::math::Aabb HumanoidBox() {
  return afw::math::Aabb{.min = {.x = -0.6F, .y = -0.6F, .z = 0.0F},
                         .max = {.x = 0.6F, .y = 0.6F, .z = 1.7F}};
}

[[nodiscard]] afw::games::fc3::RawEntity Sane() {
  return afw::games::fc3::RawEntity{.address = 0x1000,
                                    .vtable = 0x2000,
                                    .transform = RigidAt(100.0F, 200.0F, 30.0F),
                                    .box = HumanoidBox()};
}

}  // namespace

TEST(Fc3ClassifyUnit, AcceptsARigidTransformWithASaneBoxInTheWorld) {
  const auto kClassified{afw::games::fc3::Classify(Sane())};

  ASSERT_TRUE(kClassified.has_value());
  EXPECT_EQ(kClassified->address, 0x1000U);
  EXPECT_EQ(kClassified->vtable, 0x2000U);
  EXPECT_FLOAT_EQ(kClassified->transform.Translation().x, 100.0F);
}

// Nothing has been read from the entity yet at this point, so the record must
// come back with no pawn rather than with an empty one.
TEST(Fc3ClassifyUnit, LeavesThePawnRecordAbsent) {
  const auto kClassified{afw::games::fc3::Classify(Sane())};

  ASSERT_TRUE(kClassified.has_value());
  EXPECT_FALSE(kClassified->pawn.has_value());
}

TEST(Fc3ClassifyUnit, RejectsANullVtable) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.vtable = 0;

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_FALSE(kClassified.has_value());
  EXPECT_EQ(kClassified.error(), afw::games::fc3::Rejection::NoVtable);
}

// A component's bytes at the same offsets are not a transform, and a scaled
// basis is the cheapest stand-in for that.
TEST(Fc3ClassifyUnit, RejectsAScaledBasis) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.transform.cells.at(0) = 2.0F;

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_FALSE(kClassified.has_value());
  EXPECT_EQ(kClassified.error(), afw::games::fc3::Rejection::NotRigid);
}

TEST(Fc3ClassifyUnit, RejectsASwappedBox) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.box.min.z = 5.0F;

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_FALSE(kClassified.has_value());
  EXPECT_EQ(kClassified.error(), afw::games::fc3::Rejection::BadBox);
}

TEST(Fc3ClassifyUnit, RejectsTheTemplateSpawnPoint) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.transform = RigidAt(-5248.0F, -5248.0F, 0.0F);

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_FALSE(kClassified.has_value());
  EXPECT_EQ(kClassified.error(), afw::games::fc3::Rejection::TemplateSpawn);
}

// Only both axes together mean a template, so one of them alone is a place an
// entity may legitimately stand.
TEST(Fc3ClassifyUnit, AcceptsTheTemplateCoordinateOnOneAxisAlone) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.transform = RigidAt(-5248.0F, 120.0F, 0.0F);

  EXPECT_TRUE(afw::games::fc3::Classify(raw).has_value());
}

TEST(Fc3ClassifyUnit, RejectsANonFinitePosition) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.transform = RigidAt(std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F);

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_FALSE(kClassified.has_value());
  EXPECT_EQ(kClassified.error(), afw::games::fc3::Rejection::NonFinite);
}

TEST(Fc3ClassifyUnit, RejectsAPositionBeyondTheWorld) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.transform = RigidAt(1.0e9F, 0.0F, 0.0F);

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_FALSE(kClassified.has_value());
  EXPECT_EQ(kClassified.error(), afw::games::fc3::Rejection::BeyondTheWorld);
}

TEST(Fc3ClassifyUnit, MarksAHumanoidShapedBox) {
  const auto kClassified{afw::games::fc3::Classify(Sane())};

  ASSERT_TRUE(kClassified.has_value());
  EXPECT_TRUE(kClassified->humanoid);
}

// Shape only: something as tall as a person but far too wide is not one.
TEST(Fc3ClassifyUnit, DoesNotMarkAWideBoxOfTheSameHeight) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.box.max.x = 4.0F;

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_TRUE(kClassified.has_value());
  EXPECT_FALSE(kClassified->humanoid);
}

TEST(Fc3ClassifyUnit, DoesNotMarkABoxThatDoesNotRestOnItsBase) {
  afw::games::fc3::RawEntity raw{Sane()};
  raw.box.min.z = -1.0F;

  const auto kClassified{afw::games::fc3::Classify(raw)};

  ASSERT_TRUE(kClassified.has_value());
  EXPECT_FALSE(kClassified->humanoid);
}
