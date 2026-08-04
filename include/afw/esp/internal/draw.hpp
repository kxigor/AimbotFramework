#pragma once

#include <imgui.h>

#include <afw/esp/projection.hpp>
#include <optional>

namespace afw::esp {

// Everything drawn over the world. The shape carries the kind as much as the
// colour does: a box for people and vehicles, a ring for animals, a triangle
// for birds, a dot for anything the engine would not name.
void DrawEntities(const Vec3& eye, const Matrix4& projection,
                  const ImVec2& display);

void DrawMarker(const std::optional<ScreenPoint>& point, ImU32 colour);

void DrawCrosshair(const std::optional<ScreenPoint>& point, ImU32 colour);

}  // namespace afw::esp
