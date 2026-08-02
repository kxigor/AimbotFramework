#pragma once

#include <afw/math/vec3.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace afw::math {

constexpr std::size_t kOrder{4};
constexpr std::size_t kCellCount{kOrder * kOrder};

struct Matrix4 {
  [[nodiscard]] float Cell(std::size_t row, std::size_t column) const noexcept {
    return cells.at((row * kOrder) + column);
  }

  [[nodiscard]] Vec3 Row(std::size_t index) const noexcept {
    return Vec3{.x = Cell(index, 0), .y = Cell(index, 1), .z = Cell(index, 2)};
  }

  [[nodiscard]] Vec3 Translation() const noexcept { return Row(kOrder - 1); }

  [[nodiscard]] bool IsRigid(float unitTolerance,
                             float axisTolerance) const noexcept {
    const Vec3 kAxisX{Row(0)};
    const Vec3 kAxisY{Row(1)};
    const Vec3 kAxisZ{Row(2)};

    if (!kAxisX.IsUnitLength(unitTolerance) ||
        !kAxisY.IsUnitLength(unitTolerance) ||
        !kAxisZ.IsUnitLength(unitTolerance)) {
      return false;
    }

    return std::fabs(kAxisX.Dot(kAxisY)) < axisTolerance &&
           std::fabs(kAxisX.Dot(kAxisZ)) < axisTolerance &&
           std::fabs(kAxisY.Dot(kAxisZ)) < axisTolerance;
  }

  [[nodiscard]] Vec3 TransformPoint(const Vec3& local) const noexcept {
    return Translation() + (Row(0) * local.x) + (Row(1) * local.y) +
           (Row(2) * local.z);
  }

  std::array<float, kCellCount> cells{};
};

[[nodiscard]] inline Matrix4 Multiply(const Matrix4& lhs,
                                      const Matrix4& rhs) noexcept {
  Matrix4 product{};

  for (std::size_t row{0}; row < kOrder; ++row) {
    for (std::size_t column{0}; column < kOrder; ++column) {
      float sum{0.0F};
      for (std::size_t step{0}; step < kOrder; ++step) {
        sum += lhs.Cell(row, step) * rhs.Cell(step, column);
      }
      product.cells.at((row * kOrder) + column) = sum;
    }
  }

  return product;
}

static_assert(std::is_trivially_copyable_v<Matrix4>);

}  // namespace afw::math
