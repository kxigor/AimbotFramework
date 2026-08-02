#pragma once

#include <cmath>
#include <type_traits>

namespace afw::math {

// Also the wire format: engine structures store three consecutive floats, and
// afw::process::ReadAt copies straight into this. That is why the type must
// stay an aggregate with no user-declared constructor, no private members and
// no virtuals -- methods and operators are free to add, those three are not.
struct Vec3 {
  // Public by requirement, not by omission: making them private would end the
  // aggregate and take ReadAt and every Vec3{.x = ...} with it.
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  float x{};
  float y{};
  float z{};
  // NOLINTEND(misc-non-private-member-variables-in-classes)

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

  // The tolerance is a parameter rather than a constant so no caller's notion
  // of "close enough" ends up baked in here.
  [[nodiscard]] bool IsUnitLength(float tolerance) const noexcept {
    return std::fabs(Length() - 1.0F) < tolerance;
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
