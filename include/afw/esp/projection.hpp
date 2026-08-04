#pragma once

#include <afw/games/fc3/fc3.hpp>
#include <afw/math/matrix4.hpp>
#include <afw/math/vec3.hpp>
#include <optional>

namespace afw::esp {

using math::Matrix4;
using math::Vec3;

constexpr float kDrawDistance{150.0F};

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

[[nodiscard]] Matrix4 BuildView(const Vec3& eye, const Vec3& forward,
                                const Vec3& upAxis, const Vec3& right) noexcept;

[[nodiscard]] Matrix4 BuildProjection(float verticalFovRadians, float aspect,
                                      float nearPlane, float farPlane) noexcept;

[[nodiscard]] std::optional<ScreenPoint> WorldToScreen(
    const Matrix4& viewProjection, const Vec3& world, float width,
    float height) noexcept;

[[nodiscard]] float VerticalFov(float horizontalRadians, float aspect) noexcept;

[[nodiscard]] std::optional<ScreenRect> ProjectBox(
    const games::fc3::EntityInfo& entity, const Matrix4& projection,
    float width, float height) noexcept;

[[nodiscard]] Matrix4 FrameAt(const Matrix4& orientation,
                              const Vec3& centre) noexcept;

[[nodiscard]] std::optional<ScreenRect> ProjectCube(const Matrix4& frame,
                                                    float halfExtent,
                                                    const Matrix4& projection,
                                                    float width,
                                                    float height) noexcept;

}  // namespace afw::esp
