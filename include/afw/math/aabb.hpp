#pragma once

#include <afw/math/vec3.hpp>
#include <cstddef>
#include <type_traits>

namespace afw::math {

constexpr std::size_t kAabbCorners{8};

constexpr std::size_t kCornerBitX{1U};
constexpr std::size_t kCornerBitY{2U};
constexpr std::size_t kCornerBitZ{4U};

struct Aabb {
  [[nodiscard]] Vec3 Extent() const noexcept { return max - min; }

  [[nodiscard]] bool IsValid(float maxSpan) const noexcept {
    const Vec3 kExtent{Extent()};

    return kExtent.IsFinite() && kExtent.x >= 0.0F && kExtent.y >= 0.0F &&
           kExtent.z >= 0.0F && kExtent.IsWithin(maxSpan);
  }

  [[nodiscard]] Vec3 Corner(std::size_t index) const noexcept {
    return Vec3{.x = ((index & kCornerBitX) != 0U) ? max.x : min.x,
                .y = ((index & kCornerBitY) != 0U) ? max.y : min.y,
                .z = ((index & kCornerBitZ) != 0U) ? max.z : min.z};
  }
  Vec3 min{};
  Vec3 max{};
};

static_assert(std::is_trivially_copyable_v<Aabb>);

}  // namespace afw::math
