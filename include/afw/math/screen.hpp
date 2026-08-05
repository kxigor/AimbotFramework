#pragma once

#include <afw/math/angle.hpp>
#include <afw/math/matrix4.hpp>
#include <afw/math/vec3.hpp>
#include <cmath>
#include <optional>

namespace afw::math {

constexpr float kMinDepth{0.05F};
constexpr float kFullAngle{2.0F};

struct ScreenPoint {
  float x{};
  float y{};
};

struct ScreenRect {
  float left{};
  float top{};
  float right{};
  float bottom{};
};

[[nodiscard]] inline std::optional<ScreenPoint> WorldToScreen(
    const Matrix4& viewProjection, const Vec3& world, float width,
    float height) noexcept {
  const float kClipW{(world.x * viewProjection.Cell(0, 3)) +
                     (world.y * viewProjection.Cell(1, 3)) +
                     (world.z * viewProjection.Cell(2, 3)) +
                     viewProjection.Cell(3, 3)};
  if (kClipW < kMinDepth) {
    return std::nullopt;
  }

  const float kClipX{(world.x * viewProjection.Cell(0, 0)) +
                     (world.y * viewProjection.Cell(1, 0)) +
                     (world.z * viewProjection.Cell(2, 0)) +
                     viewProjection.Cell(3, 0)};
  const float kClipY{(world.x * viewProjection.Cell(0, 1)) +
                     (world.y * viewProjection.Cell(1, 1)) +
                     (world.z * viewProjection.Cell(2, 1)) +
                     viewProjection.Cell(3, 1)};

  return ScreenPoint{.x = ((kClipX / kClipW) + 1.0F) * kHalf * width,
                     .y = (1.0F - (kClipY / kClipW)) * kHalf * height};
}

[[nodiscard]] inline float VerticalFov(float horizontalRadians,
                                       float aspect) noexcept {
  return kFullAngle * std::atan(std::tan(horizontalRadians * kHalf) / aspect);
}

}  // namespace afw::math
