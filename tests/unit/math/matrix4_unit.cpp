#include <gtest/gtest.h>

#include <afw/math/matrix4.hpp>
#include <afw/math/vec3.hpp>

namespace {

constexpr float kUnitTolerance{0.05F};
constexpr float kAxisTolerance{0.05F};

// Row-major, row-vector: rows 0..2 are the axes, row 3 the translation.
[[nodiscard]] afw::math::Matrix4 Identity() {
  afw::math::Matrix4 matrix{};
  matrix.cells = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                  0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

  return matrix;
}

}  // namespace

TEST(Matrix4Unit, RowReadsTheFirstThreeCellsOfThatRow) {
  afw::math::Matrix4 matrix{Identity()};
  matrix.cells.at(12) = 7.0F;
  matrix.cells.at(13) = 8.0F;
  matrix.cells.at(14) = 9.0F;

  const afw::math::Vec3 kRow{matrix.Row(3)};

  EXPECT_FLOAT_EQ(kRow.x, 7.0F);
  EXPECT_FLOAT_EQ(kRow.y, 8.0F);
  EXPECT_FLOAT_EQ(kRow.z, 9.0F);
}

TEST(Matrix4Unit, TranslationIsTheLastRow) {
  afw::math::Matrix4 matrix{Identity()};
  matrix.cells.at(12) = -4.0F;
  matrix.cells.at(14) = 2.5F;

  const afw::math::Vec3 kTranslation{matrix.Translation()};

  EXPECT_FLOAT_EQ(kTranslation.x, -4.0F);
  EXPECT_FLOAT_EQ(kTranslation.y, 0.0F);
  EXPECT_FLOAT_EQ(kTranslation.z, 2.5F);
}

TEST(Matrix4Unit, IsRigidAcceptsTheIdentity) {
  EXPECT_TRUE(Identity().IsRigid(kUnitTolerance, kAxisTolerance));
}

// A component's bytes at the same offsets are not a transform, and that is what
// this distinguishes.
TEST(Matrix4Unit, IsRigidRejectsAScaledBasis) {
  afw::math::Matrix4 matrix{Identity()};
  matrix.cells.at(0) = 2.0F;

  EXPECT_FALSE(matrix.IsRigid(kUnitTolerance, kAxisTolerance));
}

TEST(Matrix4Unit, IsRigidRejectsANonPerpendicularBasis) {
  afw::math::Matrix4 matrix{Identity()};
  matrix.cells.at(4) = 1.0F;
  matrix.cells.at(5) = 0.0F;

  EXPECT_FALSE(matrix.IsRigid(kUnitTolerance, kAxisTolerance));
}

TEST(Matrix4Unit, TransformPointOnTheIdentityIsTheTranslation) {
  afw::math::Matrix4 matrix{Identity()};
  matrix.cells.at(12) = 10.0F;
  matrix.cells.at(13) = 20.0F;
  matrix.cells.at(14) = 30.0F;

  const afw::math::Vec3 kWorld{
      matrix.TransformPoint(afw::math::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F})};

  EXPECT_FLOAT_EQ(kWorld.x, 11.0F);
  EXPECT_FLOAT_EQ(kWorld.y, 22.0F);
  EXPECT_FLOAT_EQ(kWorld.z, 33.0F);
}

// A quarter turn about Z sends +x to +y, which pins the row order.
TEST(Matrix4Unit, TransformPointAppliesTheRowsAsAxes) {
  afw::math::Matrix4 matrix{Identity()};
  matrix.cells = {0.0F, 1.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0F, 0.0F,
                  0.0F, 0.0F, 1.0F, 0.0F, 0.0F,  0.0F, 0.0F, 1.0F};

  const afw::math::Vec3 kWorld{
      matrix.TransformPoint(afw::math::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F})};

  EXPECT_FLOAT_EQ(kWorld.x, 0.0F);
  EXPECT_FLOAT_EQ(kWorld.y, 1.0F);
  EXPECT_FLOAT_EQ(kWorld.z, 0.0F);
}

TEST(Matrix4Unit, MultiplyByTheIdentityReturnsTheOperand) {
  afw::math::Matrix4 matrix{Identity()};
  matrix.cells.at(12) = 5.0F;

  const afw::math::Matrix4 kProduct{afw::math::Multiply(matrix, Identity())};

  for (std::size_t cell{0}; cell < afw::math::kCellCount; ++cell) {
    EXPECT_FLOAT_EQ(kProduct.cells.at(cell), matrix.cells.at(cell));
  }
}
