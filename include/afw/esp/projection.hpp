#pragma once

#include <afw/math/matrix4.hpp>
#include <afw/math/vec3.hpp>
#include <optional>

namespace afw::esp {

using math::Matrix4;
using math::Vec3;

struct ScreenPoint {
  float x{};
  float y{};
};

[[nodiscard]] Matrix4 BuildView(const Vec3& eye, const Vec3& forward,
                                const Vec3& upAxis, const Vec3& right) noexcept;

[[nodiscard]] Matrix4 BuildProjection(float verticalFovRadians, float aspect,
                                      float nearPlane, float farPlane) noexcept;

[[nodiscard]] std::optional<ScreenPoint> WorldToScreen(
    const Matrix4& viewProjection, const Vec3& world, float width,
    float height) noexcept;

}  // namespace afw::esp
