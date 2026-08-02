#include <afw/esp/projection.hpp>
#include <cmath>
#include <cstddef>
#include <optional>

namespace afw::esp {

namespace {

constexpr float kHalf{0.5F};
constexpr float kMinDepth{0.05F};

[[nodiscard]] float Dot(const Vec3& lhs, const Vec3& rhs) noexcept {
  return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

[[nodiscard]] float Cell(const Matrix4& matrix, std::size_t row,
                         std::size_t column) noexcept {
  return matrix.cells.at((row * kOrder) + column);
}

}  // namespace

Matrix4 BuildView(const Vec3& eye, const Vec3& forward, const Vec3& upAxis,
                  const Vec3& right) noexcept {
  Matrix4 view{};

  view.cells = {right.x,          upAxis.x,          forward.x,          0.0F,
                right.y,          upAxis.y,          forward.y,          0.0F,
                right.z,          upAxis.z,          forward.z,          0.0F,
                -Dot(right, eye), -Dot(upAxis, eye), -Dot(forward, eye), 1.0F};

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

Matrix4 Multiply(const Matrix4& lhs, const Matrix4& rhs) noexcept {
  Matrix4 product{};

  for (std::size_t row{0}; row < kOrder; ++row) {
    for (std::size_t column{0}; column < kOrder; ++column) {
      float sum{0.0F};
      for (std::size_t step{0}; step < kOrder; ++step) {
        sum += Cell(lhs, row, step) * Cell(rhs, step, column);
      }
      product.cells.at((row * kOrder) + column) = sum;
    }
  }

  return product;
}

std::optional<ScreenPoint> WorldToScreen(const Matrix4& viewProjection,
                                         const Vec3& world, float width,
                                         float height) noexcept {
  const float kClipW{(world.x * Cell(viewProjection, 0, 3)) +
                     (world.y * Cell(viewProjection, 1, 3)) +
                     (world.z * Cell(viewProjection, 2, 3)) +
                     Cell(viewProjection, 3, 3)};
  // w is the view-space depth, so anything nearer than this is outside the
  // frustum anyway. Rejecting only w <= 0 lets a point a few centimetres in
  // front through and the divide then throws the result into the tens of
  // thousands of pixels.
  if (kClipW < kMinDepth) {
    return std::nullopt;
  }

  const float kClipX{(world.x * Cell(viewProjection, 0, 0)) +
                     (world.y * Cell(viewProjection, 1, 0)) +
                     (world.z * Cell(viewProjection, 2, 0)) +
                     Cell(viewProjection, 3, 0)};
  const float kClipY{(world.x * Cell(viewProjection, 0, 1)) +
                     (world.y * Cell(viewProjection, 1, 1)) +
                     (world.z * Cell(viewProjection, 2, 1)) +
                     Cell(viewProjection, 3, 1)};

  const float kNdcX{kClipX / kClipW};
  const float kNdcY{kClipY / kClipW};

  return ScreenPoint{.x = (kNdcX + 1.0F) * kHalf * width,
                     .y = (1.0F - kNdcY) * kHalf * height};
}

}  // namespace afw::esp
