#include <imgui.h>

#include <afw/config/config.hpp>
#include <afw/esp/internal/classify.hpp>
#include <afw/esp/internal/draw.hpp>
#include <afw/esp/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <algorithm>
#include <array>
#include <cmath>
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
constexpr float kHeadHalf{0.13F};
constexpr float kHeadLift{0.10F};
constexpr float kHeadThickness{1.0F};
constexpr float kMarkerRadius{6.0F};
constexpr float kDegreesPerRadian{57.2957795F};
constexpr float kConeThickness{1.0F};
constexpr ImU32 kConeColour{0x50FFFFFF};
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

// Where the shot is going, which is the head on a man and the spine on an
// animal or a bird. Drawing it is worth more on the animals than on the men: a
// bird's own coordinates are nowhere near its body, so without this there is
// nothing on screen to say whether the aim is on it or under it.
//
// The head bone sits at the base of the skull and needs lifting to surround the
// head rather than hang beneath it. The spine needs no lifting: it is already
// inside the body.
void DrawAimBox(const games::fc3::EntityInfo& entity, const Matrix4& projection,
                const ImVec2& display, ImU32 colour) {
  if (!config::Current().espAimBoxes.load()) {
    return;
  }

  const std::optional<Vec3> kBone{entity.head ? entity.head : entity.body};
  if (!kBone) {
    return;
  }

  const Vec3 kCentre{
      entity.head ? *kBone + (entity.transform.Row(2) * kHeadLift) : *kBone};
  const std::optional<ScreenRect> kRect{
      ProjectCube(FrameAt(entity.transform, kCentre), kHeadHalf, projection,
                  display.x, display.y)};
  if (!kRect) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddRect(ImVec2{kRect->left, kRect->top},
                                          ImVec2{kRect->right, kRect->bottom},
                                          colour, 0.0F, 0, kHeadThickness);
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

[[nodiscard]] bool Shown(Kind kind) noexcept {
  const config::Settings& kSettings{config::Current()};
  switch (kind) {
    case Kind::Enemy:
      return kSettings.espEnemies.load();
    case Kind::Ally:
      return kSettings.espAllies.load();
    case Kind::Civilian:
      return kSettings.espCivilians.load();
    case Kind::Predator:
    case Kind::Animal:
      return kSettings.espAnimals.load();
    case Kind::Bird:
      return kSettings.espBirds.load();
    case Kind::Vehicle:
      return kSettings.espVehicles.load();
    case Kind::Prop:
      return kSettings.espProps.load();
    case Kind::Self:
    default:
      return false;
  }
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
    if (!Shown(kKind)) {
      continue;
    }

    const ImU32 kColour{ColourOf(kEntity, kKind)};
    if (kKind == Kind::Prop) {
      DrawDot(kEntity, projection, display, kColour);
      continue;
    }

    if (kKind == Kind::Bird) {
      DrawTriangle(kEntity, projection, display, kColour);
    } else if (kKind == Kind::Animal || kKind == Kind::Predator) {
      DrawRing(kEntity, projection, display, kColour);
    } else {
      DrawBox(kEntity, projection, display, kColour,
              kKind == Kind::Vehicle ? kVehicleThickness : kBoxThickness);
    }

    DrawAimBox(kEntity, projection, display, kColour);
  }
}

void DrawAimCone(const ImVec2& display, float verticalFov) {
  const config::Settings& kSettings{config::Current()};
  if (!kSettings.aimEnabled.load()) {
    return;
  }

  const float kHalfFov{verticalFov * kHalfF};
  const float kEdge{std::tan(kHalfFov)};
  if (kEdge <= 0.0F) {
    return;
  }

  // The setting names the whole cone; the ring is drawn at its half-angle. Past
  // a quarter turn there is no ring to draw at all -- the cone has opened out
  // past the screen's own edge and takes in everything on it.
  constexpr float kQuarterTurn{1.57079633F};

  const float kCone{kSettings.aimConeDegrees.load() * kHalfF /
                    kDegreesPerRadian};
  if (kCone >= kQuarterTurn) {
    return;
  }

  const float kRadius{display.y * kHalfF * std::tan(kCone) / kEdge};
  if (!std::isfinite(kRadius) || kRadius <= 0.0F) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddCircle(
      ImVec2{display.x * kHalfF, display.y * kHalfF},
      std::min(kRadius, display.y), kConeColour, 0, kConeThickness);
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
