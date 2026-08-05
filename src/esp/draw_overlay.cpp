#include <imgui.h>

#include <afw/config/config.hpp>
#include <afw/esp/internal/draw.hpp>
#include <afw/esp/internal/projection.hpp>
#include <afw/math/angle.hpp>
#include <algorithm>
#include <cmath>
#include <optional>

namespace afw::esp {

namespace {

constexpr float kConeThickness{1.0F};
constexpr ImU32 kConeColour{0x50FFFFFF};
constexpr float kCrosshairArm{11.0F};
constexpr float kCrosshairThickness{1.5F};

}  // namespace

void DrawAimCone(const ImVec2& display, float verticalFov) {
  const config::Settings& kSettings{config::Current()};
  if (!kSettings.aimEnabled.load()) {
    return;
  }

  const float kEdge{std::tan(verticalFov * math::kHalf)};
  if (kEdge <= 0.0F) {
    return;
  }

  const float kCone{math::HalfAngle(kSettings.aimConeDegrees.load())};
  if (kCone >= math::kQuarterTurn) {
    return;
  }

  const float kRadius{display.y * math::kHalf * std::tan(kCone) / kEdge};
  if (!std::isfinite(kRadius) || kRadius <= 0.0F) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddCircle(
      ImVec2{display.x * math::kHalf, display.y * math::kHalf},
      std::min(kRadius, display.y), kConeColour, 0, kConeThickness);
}

void DrawCrosshair(const std::optional<math::ScreenPoint>& point,
                   ImU32 colour) {
  if (!point) {
    return;
  }

  ImDrawList* list{ImGui::GetBackgroundDrawList()};
  list->AddLine(ImVec2{point->x - kCrosshairArm, point->y},
                ImVec2{point->x + kCrosshairArm, point->y}, colour,
                kCrosshairThickness);
  list->AddLine(ImVec2{point->x, point->y - kCrosshairArm},
                ImVec2{point->x, point->y + kCrosshairArm}, colour,
                kCrosshairThickness);
}

}  // namespace afw::esp
