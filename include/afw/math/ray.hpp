#pragma once

#include <afw/math/vec3.hpp>
#include <type_traits>

namespace afw::math {

struct Ray {
  Vec3 origin{};
  Vec3 direction{};
};

static_assert(std::is_trivially_copyable_v<Ray>);

}  // namespace afw::math
