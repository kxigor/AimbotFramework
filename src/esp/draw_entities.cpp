#include <imgui.h>

#include <afw/config/config.hpp>
#include <afw/esp/internal/draw.hpp>
#include <afw/esp/internal/palette.hpp>
#include <afw/esp/internal/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/kind.hpp>
#include <afw/math/angle.hpp>
#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>

namespace afw::esp {

namespace {

constexpr float kPropRadius{2.0F};
constexpr float kBoxThickness{1.5F};
constexpr float kVehicleThickness{2.5F};
constexpr float kLabelLift{14.0F};
constexpr std::size_t kLabelChars{64};
constexpr int kTriangleSides{3};
constexpr std::uint32_t kBlockedColour{0xFF3030C0};
constexpr float kHeadHalf{0.13F};
constexpr float kHeadThickness{1.0F};

void DrawLabel(const games::fc3::EntityInfo& entity,
               const math::ScreenRect& rect, ImU32 colour) {
  std::array<char, kLabelChars> label{};
  std::snprintf(label.data(), label.size(), "h%.1f",
                static_cast<double>(entity.box.Extent().z));
  ImGui::GetBackgroundDrawList()->AddText(
      ImVec2{rect.left, rect.top - kLabelLift}, colour, label.data());
}

void DrawAimBox(const games::fc3::EntityInfo& entity,
                const math::Matrix4& projection, const ImVec2& display,
                ImU32 colour) {
  if (!config::Current().espAimBoxes.load()) {
    return;
  }

  const std::optional<math::Vec3> kBone{entity.head ? entity.head
                                                    : entity.body};
  if (!kBone) {
    return;
  }

  const ImU32 kInk{(config::Current().aimWeightReach.load() > 0.0F &&
                    games::fc3::ReachOf(entity.address) <= 0.0F)
                       ? kBlockedColour
                       : colour};

  const std::optional<math::ScreenRect> kRect{
      math::ProjectCube(math::FrameAt(entity.transform, *kBone), kHeadHalf,
                        projection, display.x, display.y)};
  if (!kRect) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddRect(ImVec2{kRect->left, kRect->top},
                                          ImVec2{kRect->right, kRect->bottom},
                                          kInk, 0.0F, 0, kHeadThickness);
}

void DrawBox(const games::fc3::EntityInfo& entity,
             const math::Matrix4& projection, const ImVec2& display,
             ImU32 colour, float thickness) {
  const std::optional<math::ScreenRect> kRect{
      ProjectBox(entity, projection, display.x, display.y)};
  if (!kRect) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddRect(ImVec2{kRect->left, kRect->top},
                                          ImVec2{kRect->right, kRect->bottom},
                                          colour, 0.0F, 0, thickness);
  DrawLabel(entity, *kRect, colour);
}

void DrawRing(const games::fc3::EntityInfo& entity,
              const math::Matrix4& projection, const ImVec2& display,
              ImU32 colour) {
  const std::optional<math::ScreenRect> kRect{
      ProjectBox(entity, projection, display.x, display.y)};
  if (!kRect) {
    return;
  }

  const ImVec2 kCentre{(kRect->left + kRect->right) * math::kHalf,
                       (kRect->top + kRect->bottom) * math::kHalf};
  ImGui::GetBackgroundDrawList()->AddCircle(
      kCentre, (kRect->bottom - kRect->top) * math::kHalf, colour, 0,
      kBoxThickness);
  DrawLabel(entity, *kRect, colour);
}

void DrawTriangle(const games::fc3::EntityInfo& entity,
                  const math::Matrix4& projection, const ImVec2& display,
                  ImU32 colour) {
  const std::optional<math::ScreenRect> kRect{
      ProjectBox(entity, projection, display.x, display.y)};
  if (!kRect) {
    return;
  }

  const ImVec2 kCentre{(kRect->left + kRect->right) * math::kHalf,
                       (kRect->top + kRect->bottom) * math::kHalf};
  ImGui::GetBackgroundDrawList()->AddNgon(
      kCentre, (kRect->bottom - kRect->top) * math::kHalf, colour,
      kTriangleSides, kBoxThickness);
}

void DrawDot(const games::fc3::EntityInfo& entity,
             const math::Matrix4& projection, const ImVec2& display,
             ImU32 colour) {
  const std::optional<math::ScreenPoint> kPoint{math::WorldToScreen(
      projection, entity.transform.Translation(), display.x, display.y)};
  if (kPoint) {
    ImGui::GetBackgroundDrawList()->AddCircleFilled(
        ImVec2{kPoint->x, kPoint->y}, kPropRadius, colour);
  }
}

}  // namespace

void DrawEntities(const math::Vec3& eye, const math::Matrix4& projection,
                  const ImVec2& display) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{games::fc3::FindSelf(kSnapshot)};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    if ((kEntity.transform.Translation() - eye).Length() > kDrawDistance) {
      continue;
    }

    const games::fc3::Kind kKind{games::fc3::KindOf(kEntity, kSelf)};
    if (!Shown(kKind)) {
      continue;
    }

    const ImU32 kColour{ColourOf(kEntity, kKind)};
    if (kKind == games::fc3::Kind::Prop) {
      DrawDot(kEntity, projection, display, kColour);
      continue;
    }

    if (kKind == games::fc3::Kind::Bird) {
      DrawTriangle(kEntity, projection, display, kColour);
    } else if (kKind == games::fc3::Kind::Animal ||
               kKind == games::fc3::Kind::Predator) {
      DrawRing(kEntity, projection, display, kColour);
    } else {
      DrawBox(kEntity, projection, display, kColour,
              kKind == games::fc3::Kind::Vehicle ? kVehicleThickness
                                                 : kBoxThickness);
    }

    DrawAimBox(kEntity, projection, display, kColour);
  }
}

}  // namespace afw::esp
