#pragma once

#include <afw/math/aabb.hpp>
#include <afw/math/matrix4.hpp>
#include <afw/math/screen.hpp>
#include <afw/math/vec3.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

namespace afw::math {

constexpr std::size_t kTranslationCell{(kOrder - 1) * kOrder};

[[nodiscard]] inline Matrix4 BuildView(const Vec3& eye, const Vec3& forward,
                                       const Vec3& upAxis,
                                       const Vec3& right) noexcept {
  Matrix4 view{};

  view.cells = {right.x,         upAxis.x,         forward.x,         0.0F,
                right.y,         upAxis.y,         forward.y,         0.0F,
                right.z,         upAxis.z,         forward.z,         0.0F,
                -right.Dot(eye), -upAxis.Dot(eye), -forward.Dot(eye), 1.0F};

  return view;
}

[[nodiscard]] inline Matrix4 BuildProjection(float verticalFovRadians,
                                             float aspect, float nearPlane,
                                             float farPlane) noexcept {
  const float kYScale{1.0F / std::tan(verticalFovRadians * kHalf)};
  const float kXScale{kYScale / aspect};
  const float kDepth{farPlane / (farPlane - nearPlane)};

  Matrix4 projection{};

  projection.cells = {kXScale, 0.0F, 0.0F, 0.0F, 0.0F,
                      kYScale, 0.0F, 0.0F, 0.0F, 0.0F,
                      kDepth,  1.0F, 0.0F, 0.0F, -nearPlane * kDepth,
                      0.0F};

  return projection;
}

[[nodiscard]] inline Matrix4 FrameAt(const Matrix4& orientation,
                                     const Vec3& centre) noexcept {
  Matrix4 frame{orientation};
  frame.cells.at(kTranslationCell) = centre.x;
  frame.cells.at(kTranslationCell + 1) = centre.y;
  frame.cells.at(kTranslationCell + 2) = centre.z;

  return frame;
}

[[nodiscard]] inline std::optional<ScreenRect> ProjectCorners(
    const Aabb& box, const Matrix4& frame, const Matrix4& projection,
    float width, float height) noexcept {
  ScreenRect rect{.left = std::numeric_limits<float>::max(),
                  .top = std::numeric_limits<float>::max(),
                  .right = std::numeric_limits<float>::lowest(),
                  .bottom = std::numeric_limits<float>::lowest()};

  for (std::size_t corner{0}; corner < kAabbCorners; ++corner) {
    const std::optional<ScreenPoint> kPoint{WorldToScreen(
        projection, frame.TransformPoint(box.Corner(corner)), width, height)};
    if (!kPoint) {
      return std::nullopt;
    }

    rect.left = std::min(rect.left, kPoint->x);
    rect.right = std::max(rect.right, kPoint->x);
    rect.top = std::min(rect.top, kPoint->y);
    rect.bottom = std::max(rect.bottom, kPoint->y);
  }

  if (rect.right <= rect.left || rect.bottom <= rect.top) {
    return std::nullopt;
  }

  return rect;
}

[[nodiscard]] inline std::optional<ScreenRect> ProjectCube(
    const Matrix4& frame, float halfExtent, const Matrix4& projection,
    float width, float height) noexcept {
  const Aabb kCube{
      .min = Vec3{.x = -halfExtent, .y = -halfExtent, .z = -halfExtent},
      .max = Vec3{.x = halfExtent, .y = halfExtent, .z = halfExtent}};

  return ProjectCorners(kCube, frame, projection, width, height);
}

}  // namespace afw::math
