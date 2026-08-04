#include <afw/esp/projection.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

namespace afw::esp {

namespace {

constexpr float kHalf{0.5F};
constexpr float kMinDepth{0.05F};
constexpr float kFullAngle{2.0F};

}  // namespace

Matrix4 BuildView(const Vec3& eye, const Vec3& forward, const Vec3& upAxis,
                  const Vec3& right) noexcept {
  Matrix4 view{};

  view.cells = {right.x,         upAxis.x,         forward.x,         0.0F,
                right.y,         upAxis.y,         forward.y,         0.0F,
                right.z,         upAxis.z,         forward.z,         0.0F,
                -right.Dot(eye), -upAxis.Dot(eye), -forward.Dot(eye), 1.0F};

  return view;
}

Matrix4 BuildProjection(float verticalFovRadians, float aspect, float nearPlane,
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

std::optional<ScreenPoint> WorldToScreen(const Matrix4& viewProjection,
                                         const Vec3& world, float width,
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

  const float kNdcX{kClipX / kClipW};
  const float kNdcY{kClipY / kClipW};

  return ScreenPoint{.x = (kNdcX + 1.0F) * kHalf * width,
                     .y = (1.0F - kNdcY) * kHalf * height};
}

float VerticalFov(float horizontalRadians, float aspect) noexcept {
  return kFullAngle * std::atan(std::tan(horizontalRadians * kHalf) / aspect);
}

std::optional<ScreenRect> ProjectBox(const games::fc3::EntityInfo& entity,
                                     const Matrix4& projection, float width,
                                     float height) noexcept {
  ScreenRect rect{.left = std::numeric_limits<float>::max(),
                  .top = std::numeric_limits<float>::max(),
                  .right = std::numeric_limits<float>::lowest(),
                  .bottom = std::numeric_limits<float>::lowest()};

  for (std::size_t corner{0}; corner < math::kAabbCorners; ++corner) {
    const std::optional<ScreenPoint> kPoint{WorldToScreen(
        projection, entity.transform.TransformPoint(entity.box.Corner(corner)),
        width, height)};
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

}  // namespace afw::esp
