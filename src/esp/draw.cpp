#include <imgui.h>

#include <afw/esp/internal/classify.hpp>
#include <afw/esp/internal/draw.hpp>
#include <afw/esp/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>

namespace afw::esp {

namespace {

constexpr float kHalfF{0.5F};
constexpr float kPropRadius{2.0F};
constexpr float kBoxThickness{1.5F};
constexpr float kVehicleThickness{2.5F};
constexpr float kLabelLift{14.0F};
constexpr std::size_t kLabelChars{64};
constexpr int kTriangleSides{3};
constexpr float kMarkerRadius{6.0F};
constexpr float kCrosshairArm{11.0F};
constexpr float kCrosshairThickness{1.5F};

void DrawLabel(const games::fc3::EntityInfo& entity, const ScreenRect& rect,
               ImU32 colour) {
  std::array<char, kLabelChars> label{};
  std::snprintf(label.data(), label.size(), "h%.1f",
                static_cast<double>(entity.box.Extent().z));
  ImGui::GetBackgroundDrawList()->AddText(
      ImVec2{rect.left, rect.top - kLabelLift}, colour, label.data());
}

void DrawBox(const games::fc3::EntityInfo& entity, const Matrix4& projection,
             const ImVec2& display, ImU32 colour, float thickness) {
  const std::optional<ScreenRect> kRect{
      ProjectBox(entity, projection, display.x, display.y)};
  if (!kRect) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddRect(ImVec2{kRect->left, kRect->top},
                                          ImVec2{kRect->right, kRect->bottom},
                                          colour, 0.0F, 0, thickness);
  DrawLabel(entity, *kRect, colour);
}

// The radius comes from the height alone. Taken from the wider side it swung by
// half as the animal turned, because a boar's box is far longer than it is
// tall.
void DrawRing(const games::fc3::EntityInfo& entity, const Matrix4& projection,
              const ImVec2& display, ImU32 colour) {
  const std::optional<ScreenRect> kRect{
      ProjectBox(entity, projection, display.x, display.y)};
  if (!kRect) {
    return;
  }

  const ImVec2 kCentre{(kRect->left + kRect->right) * kHalfF,
                       (kRect->top + kRect->bottom) * kHalfF};
  ImGui::GetBackgroundDrawList()->AddCircle(
      kCentre, (kRect->bottom - kRect->top) * kHalfF, colour, 0, kBoxThickness);
  DrawLabel(entity, *kRect, colour);
}

void DrawTriangle(const games::fc3::EntityInfo& entity,
                  const Matrix4& projection, const ImVec2& display,
                  ImU32 colour) {
  const std::optional<ScreenRect> kRect{
      ProjectBox(entity, projection, display.x, display.y)};
  if (!kRect) {
    return;
  }

  const ImVec2 kCentre{(kRect->left + kRect->right) * kHalfF,
                       (kRect->top + kRect->bottom) * kHalfF};
  ImGui::GetBackgroundDrawList()->AddNgon(
      kCentre, (kRect->bottom - kRect->top) * kHalfF, colour, kTriangleSides,
      kBoxThickness);
}

void DrawDot(const games::fc3::EntityInfo& entity, const Matrix4& projection,
             const ImVec2& display, ImU32 colour) {
  const std::optional<ScreenPoint> kPoint{WorldToScreen(
      projection, entity.transform.Translation(), display.x, display.y)};
  if (kPoint) {
    ImGui::GetBackgroundDrawList()->AddCircleFilled(
        ImVec2{kPoint->x, kPoint->y}, kPropRadius, colour);
  }
}

}  // namespace

void DrawEntities(const Vec3& eye, const Matrix4& projection,
                  const ImVec2& display) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{FindSelf(kSnapshot)};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    if ((kEntity.transform.Translation() - eye).Length() > kDrawDistance) {
      continue;
    }

    const Kind kKind{KindOf(kEntity, kSelf)};
    if (kKind == Kind::Self) {
      continue;
    }

    const ImU32 kColour{ColourOf(kEntity, kKind)};
    if (kKind == Kind::Bird) {
      DrawTriangle(kEntity, projection, display, kColour);
      continue;
    }
    if (kKind == Kind::Animal || kKind == Kind::Predator) {
      DrawRing(kEntity, projection, display, kColour);
      continue;
    }
    if (kKind == Kind::Prop) {
      DrawDot(kEntity, projection, display, kColour);
      continue;
    }

    DrawBox(kEntity, projection, display, kColour,
            kKind == Kind::Vehicle ? kVehicleThickness : kBoxThickness);
  }
}

void DrawMarker(const std::optional<ScreenPoint>& point, ImU32 colour) {
  if (!point) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2{point->x, point->y},
                                                  kMarkerRadius, colour);
}

void DrawCrosshair(const std::optional<ScreenPoint>& point, ImU32 colour) {
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
