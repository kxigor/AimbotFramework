#pragma once

#include <afw/games/fc3/fc3.hpp>
#include <afw/math/matrix4.hpp>
#include <afw/math/projection.hpp>
#include <afw/math/screen.hpp>
#include <afw/math/vec3.hpp>
#include <optional>

namespace afw::esp {

constexpr float kDrawDistance{150.0F};

[[nodiscard]] inline std::optional<math::ScreenRect> ProjectBox(
    const games::fc3::EntityInfo& entity, const math::Matrix4& projection,
    float width, float height) noexcept {
  return math::ProjectCorners(entity.box, entity.transform, projection, width,
                              height);
}

}  // namespace afw::esp
