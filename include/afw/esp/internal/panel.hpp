#pragma once

#include <imgui.h>

#include <afw/esp/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <cstdint>
#include <optional>

namespace afw::esp {

// The diagnostic panel. None of it draws over the world and none of it is read
// by anything that does -- it exists so that a wrong colour can be traced to
// the field it came from, which is how every classification bug in this game
// was actually found.

void ShowAddress(const char* label, std::uintptr_t value);

void ShowAxis(const char* label, const Vec3& axis);

void ShowVector(const char* label, const Vec3& value);

void ShowPoint(const char* label, const std::optional<ScreenPoint>& point);

void ShowChain(const games::fc3::Diagnostics& report, const ImVec2& display);

void ShowCamera(const games::fc3::CameraBasis& camera, const Vec3& right,
                float aspect, float verticalFov);

void ShowMarkers(const ImVec2& display, const std::optional<ScreenPoint>& ahead,
                 const std::optional<ScreenPoint>& probe);

// What is under the crosshair and what stands nearby, with every pawn field
// spelled out.
void ShowEntities(const Vec3& eye, const Vec3& forward);

// Where the entities live and which classes they are, for when the census
// itself looks wrong.
void ShowVtables();

void ShowPools(const Vec3& eye);

}  // namespace afw::esp
