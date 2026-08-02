#pragma once

#include <afw/math/vec3.hpp>
#include <array>
#include <cstddef>
#include <optional>

namespace afw::esp {

constexpr std::size_t kOrder{4};
constexpr std::size_t kCellCount{kOrder * kOrder};

using math::Vec3;

struct ScreenPoint {
  float x{};
  float y{};
};

// Row-major storage, row-vector convention: a point is transformed as v * M.
struct Matrix4 {
  std::array<float, kCellCount> cells{};
};

[[nodiscard]] Matrix4 BuildView(const Vec3& eye, const Vec3& forward,
                                const Vec3& upAxis, const Vec3& right) noexcept;

[[nodiscard]] Matrix4 BuildProjection(float verticalFovRadians, float aspect,
                                      float nearPlane, float farPlane) noexcept;

[[nodiscard]] Matrix4 Multiply(const Matrix4& lhs, const Matrix4& rhs) noexcept;

[[nodiscard]] std::optional<ScreenPoint> WorldToScreen(
    const Matrix4& viewProjection, const Vec3& world, float width,
    float height) noexcept;

}  // namespace afw::esp
