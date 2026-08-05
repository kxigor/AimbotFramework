#pragma once

#include <imgui.h>

#include <afw/esp/projection.hpp>
#include <optional>

namespace afw::esp {

// Everything drawn over the world. The shape carries the kind as much as the
// colour does: a box for people and vehicles, a ring for animals, a triangle
// for birds, a dot for anything the engine would not name.
void DrawEntities(const math::Vec3& eye, const math::Matrix4& projection,
                  const ImVec2& display);

void DrawCrosshair(const std::optional<math::ScreenPoint>& point, ImU32 colour);

// The reach of the aim assist, as the ring it actually is. The cone is an angle
// from the view axis, so its radius on screen follows from the projection
// rather than from any pixel count: nothing inside the ring is out of reach,
// and nothing outside it is considered.
void DrawAimCone(const ImVec2& display, float verticalFov);

}  // namespace afw::esp
