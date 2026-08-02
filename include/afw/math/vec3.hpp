#pragma once

#include <cmath>
#include <type_traits>

namespace afw::math {

struct Vec3 {
  [[nodiscard]] float Dot(const Vec3& other) const noexcept {
    return (x * other.x) + (y * other.y) + (z * other.z);
  }

  [[nodiscard]] Vec3 Cross(const Vec3& other) const noexcept {
    return Vec3{.x = (y * other.z) - (z * other.y),
                .y = (z * other.x) - (x * other.z),
                .z = (x * other.y) - (y * other.x)};
  }

  [[nodiscard]] float Length() const noexcept { return std::sqrt(Dot(*this)); }

  [[nodiscard]] Vec3 Normalised() const noexcept {
    const float kLength{Length()};
    if (kLength <= 0.0F) {
      return Vec3{};
    }

    return Vec3{.x = x / kLength, .y = y / kLength, .z = z / kLength};
  }

  [[nodiscard]] bool IsUnitLength(float tolerance) const noexcept {
    return std::fabs(Length() - 1.0F) < tolerance;
  }

  [[nodiscard]] bool IsFinite() const noexcept {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
  }

  [[nodiscard]] bool IsWithin(float bound) const noexcept {
    return std::fabs(x) <= bound && std::fabs(y) <= bound &&
           std::fabs(z) <= bound;
  }

  Vec3& operator+=(const Vec3& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;

    return *this;
  }

  Vec3& operator-=(const Vec3& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;
  }

  Vec3& operator*=(float scale) noexcept {
    x *= scale;
    y *= scale;
    z *= scale;

    return *this;
  }
  float x{};
  float y{};
  float z{};
};

[[nodiscard]] inline Vec3 operator+(Vec3 lhs, const Vec3& rhs) noexcept {
  return lhs += rhs;
}

[[nodiscard]] inline Vec3 operator-(Vec3 lhs, const Vec3& rhs) noexcept {
  return lhs -= rhs;
}

[[nodiscard]] inline Vec3 operator*(Vec3 lhs, float scale) noexcept {
  return lhs *= scale;
}

static_assert(std::is_trivially_copyable_v<Vec3>);

}  // namespace afw::math
