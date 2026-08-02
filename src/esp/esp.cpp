#include <imgui.h>

#include <afw/config/config.hpp>
#include <afw/esp/esp.hpp>
#include <afw/esp/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/render/overlay.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <system_error>

namespace afw::esp {

namespace {

constexpr float kNearPlane{0.1F};
constexpr float kFarPlane{10000.0F};
constexpr float kMarkerRadius{6.0F};
constexpr float kAheadDistance{2.0F};
constexpr float kProbeDistance{10.0F};
constexpr float kProbeOffset{2.0F};

constexpr double kHalf{0.5};
constexpr float kHalfF{0.5F};
constexpr double kDegreesPerHalfTurn{180.0};
constexpr float kFullAngle{2.0F};

constexpr ImU32 kAheadColour{0xFF00FF00};
constexpr ImU32 kRightColour{0xFF0080FF};
constexpr ImU32 kPropColour{0xC000FF40};
constexpr ImU32 kDeadColour{0x80606060};
// Humanoid by box shape but not confirmed a pawn by class.
constexpr ImU32 kHumanoidColour{0xFF00D0FF};

// By army number: Blue, Red, Grey, Neutral, Green, Yellow. Friend or foe needs
// the local player's army, which is not read yet.
constexpr std::array<ImU32, 6> kArmyColours{0xFFFF6030, 0xFF3030FF, 0xFFA0A0A0,
                                            0xFFFFFFFF, 0xFF30FF30, 0xFF30FFFF};

constexpr float kDrawDistance{150.0F};
constexpr float kPropRadius{2.0F};
constexpr std::size_t kBoxCorners{8};
constexpr float kBoxThickness{1.5F};
constexpr float kLabelLift{14.0F};
constexpr std::size_t kLabelChars{64};
constexpr std::size_t kVtableSlots{12};
constexpr float kAimTolerance{0.95F};
constexpr float kPanelWidth{470.0F};
constexpr float kPanelHeight{820.0F};

[[nodiscard]] Vec3 ToVec3(const games::fc3::Triple& triple) noexcept {
  return Vec3{.x = triple.at(0), .y = triple.at(1), .z = triple.at(2)};
}

[[nodiscard]] Vec3 Offset(const Vec3& origin, const Vec3& direction,
                          float distance) noexcept {
  return Vec3{.x = origin.x + (direction.x * distance),
              .y = origin.y + (direction.y * distance),
              .z = origin.z + (direction.z * distance)};
}

// The engine derives its vertical half-angle from the horizontal one.
[[nodiscard]] float VerticalFov(float horizontalRadians,
                                float aspect) noexcept {
  return kFullAngle * std::atan(std::tan(horizontalRadians * kHalfF) / aspect);
}

[[nodiscard]] float Length(const Vec3& value) noexcept {
  return std::sqrt((value.x * value.x) + (value.y * value.y) +
                   (value.z * value.z));
}

void ShowAddress(const char* label, std::uintptr_t value) {
  if (value == 0) {
    ImGui::Text("%-18s --", label);
    return;
  }
  ImGui::Text("%-18s %llX", label, static_cast<std::uint64_t>(value));
}

void ShowAxis(const char* label, const Vec3& axis) {
  ImGui::Text("%-18s %8.3f %8.3f %8.3f  |%.3f|", label,
              static_cast<double>(axis.x), static_cast<double>(axis.y),
              static_cast<double>(axis.z), static_cast<double>(Length(axis)));
}

void ShowPoint(const char* label, const std::optional<ScreenPoint>& point) {
  if (!point) {
    ImGui::Text("%-18s behind the camera", label);
    return;
  }
  ImGui::Text("%-18s %8.1f %8.1f", label, static_cast<double>(point->x),
              static_cast<double>(point->y));
}

void ShowChain(const games::fc3::Diagnostics& report, const ImVec2& display) {
  ImGui::SeparatorText("chain");
  ShowAddress("module base", report.moduleBase);
  ShowAddress("container addr", report.containerAddress);
  ShowAddress("container", report.container);
  ImGui::Text("%-18s %d", "live flag", report.liveFlag);
  ShowAddress("manager", report.manager);
  ShowAddress("camera view", report.cameraView);

  ImGui::SeparatorText("viewport");
  ShowAddress("block addr", report.viewportAddress);
  ShowAddress("block", report.viewportBlock);
  ImGui::Text("%-18s %d x %d  (not used)", "game render", report.viewportWidth,
              report.viewportHeight);
  const render::SurfaceSize kBackBuffer{render::BackBufferSize()};
  ImGui::Text("%-18s %u x %u", "back buffer", kBackBuffer.width,
              kBackBuffer.height);
  ImGui::Text("%-18s %.0f x %.0f  (used)", "imgui display",
              static_cast<double>(display.x), static_cast<double>(display.y));

  const games::fc3::EntityCensus kCensus{games::fc3::LastCensus()};
  ImGui::SeparatorText("entities");
  ImGui::Text("%-18s %u", "update frames", kCensus.frames);
  ImGui::Text("%-18s %u", "passes / frame", kCensus.calls);
  ImGui::Text("%-18s %u", "groups", kCensus.groups);
  ImGui::Text("%-18s %u", "entities", kCensus.entities);
  ImGui::Text("%-18s %u", "groups skipped", kCensus.skipped);

  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  ImGui::Text("%-18s %u", "in the world", kSnapshot.count);
  ImGui::Text("%-18s %u", "over capacity", kSnapshot.dropped);
  ImGui::Text("%-18s %u", "not entities", kSnapshot.notEntities);
  ImGui::Text("%-18s %u", "humanoid boxes", kSnapshot.humanoids);
  ImGui::Text("%-18s %u", "pawns by class", kSnapshot.pawns);
}

void ShowAngle(const char* label, float radians) {
  ImGui::Text(
      "%-18s %.4f rad = %.2f deg", label, static_cast<double>(radians),
      static_cast<double>(radians) * kDegreesPerHalfTurn / std::numbers::pi);
}

void ShowCamera(const games::fc3::CameraBasis& camera, const Vec3& right,
                float aspect, float verticalFov) {
  ShowAxis("position", ToVec3(camera.position));
  ShowAxis("forward", ToVec3(camera.forward));
  ShowAxis("up", ToVec3(camera.upAxis));
  ShowAxis("right", right);
  ShowAngle("fov h (engine)", camera.fovHorizontalRadians);
  ShowAngle("fov v (derived)", verticalFov);
  ShowAngle("fov v (field)", camera.fovRadians);
  ImGui::Text("%-18s %.4f", "aspect", static_cast<double>(aspect));
}

void ShowMarkers(const ImVec2& display, const std::optional<ScreenPoint>& ahead,
                 const std::optional<ScreenPoint>& probe) {
  ImGui::SeparatorText("markers");
  ImGui::Text("%-18s %8.1f %8.1f", "screen centre",
              static_cast<double>(display.x) * kHalf,
              static_cast<double>(display.y) * kHalf);
  ShowPoint("ahead 2m", ahead);
  ShowPoint("right 2m at 10m", probe);
}

[[nodiscard]] float DistanceTo(const Vec3& eye,
                               const games::fc3::Triple& point) noexcept {
  const Vec3 kDelta{.x = point.at(0) - eye.x,
                    .y = point.at(1) - eye.y,
                    .z = point.at(2) - eye.z};

  return Length(kDelta);
}

struct ScreenRect {
  float left{};
  float top{};
  float right{};
  float bottom{};
};

// world = local * M, with M's basis rows in axisX/Y/Z and its translation row
// in position.
[[nodiscard]] Vec3 ToWorld(const games::fc3::EntityInfo& entity,
                           const Vec3& local) noexcept {
  return Vec3{
      .x = entity.position.at(0) + (entity.axisX.at(0) * local.x) +
           (entity.axisY.at(0) * local.y) + (entity.axisZ.at(0) * local.z),
      .y = entity.position.at(1) + (entity.axisX.at(1) * local.x) +
           (entity.axisY.at(1) * local.y) + (entity.axisZ.at(1) * local.z),
      .z = entity.position.at(2) + (entity.axisX.at(2) * local.x) +
           (entity.axisY.at(2) * local.y) + (entity.axisZ.at(2) * local.z)};
}

// The screen bounding rectangle of the box's eight corners, so the yaw and the
// off-centre depth spread are both accounted for.
[[nodiscard]] std::optional<ScreenRect> ProjectBox(
    const games::fc3::EntityInfo& entity, const Matrix4& projection,
    const ImVec2& display) noexcept {
  ScreenRect rect{.left = std::numeric_limits<float>::max(),
                  .top = std::numeric_limits<float>::max(),
                  .right = std::numeric_limits<float>::lowest(),
                  .bottom = std::numeric_limits<float>::lowest()};

  for (std::size_t corner{0}; corner < kBoxCorners; ++corner) {
    const Vec3 kLocal{.x = ((corner & 1U) != 0U) ? entity.aabbMax.at(0)
                                                 : entity.aabbMin.at(0),
                      .y = ((corner & 2U) != 0U) ? entity.aabbMax.at(1)
                                                 : entity.aabbMin.at(1),
                      .z = ((corner & 4U) != 0U) ? entity.aabbMax.at(2)
                                                 : entity.aabbMin.at(2)};

    const std::optional<ScreenPoint> kPoint{WorldToScreen(
        projection, ToWorld(entity, kLocal), display.x, display.y)};
    if (!kPoint) {
      return std::nullopt;
    }

    rect.left = std::min(rect.left, kPoint->x);
    rect.right = std::max(rect.right, kPoint->x);
    rect.top = std::min(rect.top, kPoint->y);
    rect.bottom = std::max(rect.bottom, kPoint->y);
  }

  if (rect.right <= rect.left || rect.bottom <= rect.top) {
    return std::nullopt;
  }

  return rect;
}

void DrawPawn(const games::fc3::EntityInfo& entity, const Matrix4& projection,
              const ImVec2& display) {
  const std::optional<ScreenRect> kRect{
      ProjectBox(entity, projection, display)};
  if (!kRect) {
    return;
  }

  ImU32 colour{kHumanoidColour};
  if (entity.dead) {
    colour = kDeadColour;
  } else if (entity.pawn) {
    colour = kArmyColours.at(static_cast<std::size_t>(entity.army) %
                             kArmyColours.size());
  }
  const ImU32 kColour{colour};

  ImGui::GetBackgroundDrawList()->AddRect(ImVec2{kRect->left, kRect->top},
                                          ImVec2{kRect->right, kRect->bottom},
                                          kColour, 0.0F, 0, kBoxThickness);

  std::array<char, kLabelChars> label{};
  std::snprintf(label.data(), label.size(), "%llX h%.1f %s%d",
                static_cast<std::uint64_t>(entity.vtable),
                static_cast<double>(entity.height),
                entity.pawn ? "a" : "shape a", entity.army);
  ImGui::GetBackgroundDrawList()->AddText(
      ImVec2{kRect->left, kRect->top - kLabelLift}, kColour, label.data());
}

void DrawEntities(const Vec3& eye, const Matrix4& projection,
                  const ImVec2& display) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    if (DistanceTo(eye, kEntity.position) > kDrawDistance) {
      continue;
    }

    if ((kEntity.pawn || kEntity.humanoid) && kEntity.height > 0.0F) {
      DrawPawn(kEntity, projection, display);
      continue;
    }

    const std::optional<ScreenPoint> kPoint{
        WorldToScreen(projection,
                      Vec3{.x = kEntity.position.at(0),
                           .y = kEntity.position.at(1),
                           .z = kEntity.position.at(2)},
                      display.x, display.y)};
    if (kPoint) {
      ImGui::GetBackgroundDrawList()->AddCircleFilled(
          ImVec2{kPoint->x, kPoint->y}, kPropRadius, kPropColour);
    }
  }
}

[[nodiscard]] Vec3 Normalise(const Vec3& value) noexcept {
  const float kLength{Length(value)};
  if (kLength <= 0.0F) {
    return Vec3{};
  }

  return Vec3{
      .x = value.x / kLength, .y = value.y / kLength, .z = value.z / kLength};
}

void ShowTriple(const char* label, const games::fc3::Triple& value) {
  ImGui::Text(
      "%-14s %9.3f %9.3f %9.3f", label, static_cast<double>(value.at(0)),
      static_cast<double>(value.at(1)), static_cast<double>(value.at(2)));
}

// Whatever sits nearest the crosshair, dumped raw.
void ShowInspector(const Vec3& eye, const Vec3& forward) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};

  const games::fc3::EntityInfo* nearest{nullptr};
  float best{kAimTolerance};
  float bestDistance{0.0F};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    const Vec3 kDelta{.x = kEntity.position.at(0) - eye.x,
                      .y = kEntity.position.at(1) - eye.y,
                      .z = kEntity.position.at(2) - eye.z};
    const float kDistance{Length(kDelta)};
    if (kDistance < 1.0F || kDistance > kDrawDistance) {
      continue;
    }

    const Vec3 kDirection{Normalise(kDelta)};
    const float kDot{(kDirection.x * forward.x) + (kDirection.y * forward.y) +
                     (kDirection.z * forward.z)};
    if (kDot > best) {
      best = kDot;
      nearest = &kEntity;
      bestDistance = kDistance;
    }
  }

  ImGui::SeparatorText("crosshair");
  if (nearest == nullptr) {
    ImGui::TextUnformatted("nothing within the aim cone");
    return;
  }

  ImGui::Text("%-14s %llX", "entity",
              static_cast<std::uint64_t>(nearest->address));
  ImGui::Text("%-14s %llX", "vtable",
              static_cast<std::uint64_t>(nearest->vtable));
  ImGui::Text("%-14s %.1f m", "distance", static_cast<double>(bestDistance));
  ShowTriple("position", nearest->position);
  ShowTriple("aabb min", nearest->aabbMin);
  ShowTriple("aabb max", nearest->aabbMax);
  ImGui::Text("%-14s %d  faction %d  dead %u  pawn %d", "army", nearest->army,
              nearest->faction, nearest->deadByte,
              static_cast<int>(nearest->pawn));
  ImGui::Text("%-14s own %02X  hostile %02X  reverse %02X", "masks",
              nearest->ownArmyBit, nearest->hostileMask,
              nearest->reverseHostileMask);
}

struct VtableTally {
  std::uintptr_t vtable{};
  std::uint32_t count{};
};

struct VtableReport {
  std::array<VtableTally, kVtableSlots> rows{};
  std::uint32_t overflow{};
};

[[nodiscard]] VtableReport CollectVtables() noexcept {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};

  VtableReport report{};
  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const std::uintptr_t kVtable{kSnapshot.items.at(index).vtable};

    bool placed{false};
    for (VtableTally& row : report.rows) {
      if (row.count == 0) {
        row = VtableTally{.vtable = kVtable, .count = 1};
        placed = true;
        break;
      }
      if (row.vtable == kVtable) {
        ++row.count;
        placed = true;
        break;
      }
    }

    if (!placed) {
      ++report.overflow;
    }
  }

  std::ranges::sort(report.rows, std::ranges::greater{}, &VtableTally::count);

  return report;
}

void ShowVtables() {
  const VtableReport kReport{CollectVtables()};

  ImGui::SeparatorText("vtables");
  for (const VtableTally& kRow : kReport.rows) {
    if (kRow.count == 0) {
      break;
    }
    ImGui::Text("%-18llX %5u", static_cast<std::uint64_t>(kRow.vtable),
                kRow.count);
  }

  if (kReport.overflow != 0) {
    ImGui::Text("%-18s %u", "beyond the table", kReport.overflow);
  }
}

// Which heap pool the walk is actually reaching.
void ShowPools(const Vec3& eye) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};

  std::uintptr_t low{0};
  std::uintptr_t high{0};
  std::uint32_t nearby{0};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    if (low == 0 || kEntity.address < low) {
      low = kEntity.address;
    }
    high = std::max(kEntity.address, high);
    if (DistanceTo(eye, kEntity.position) <= kDrawDistance) {
      ++nearby;
    }
  }

  ImGui::SeparatorText("pools");
  ImGui::Text("%-18s %llX .. %llX", "entity addresses",
              static_cast<std::uint64_t>(low),
              static_cast<std::uint64_t>(high));
  ImGui::Text("%-18s %u  of %u", "within range", nearby, kSnapshot.count);
}

struct Markers {
  std::optional<ScreenPoint> ahead;
  std::optional<ScreenPoint> probe;
};

[[nodiscard]] Markers ProjectMarkers(const Matrix4& projection, const Vec3& eye,
                                     const Vec3& forward, const Vec3& right,
                                     const ImVec2& display) noexcept {
  return Markers{
      .ahead = WorldToScreen(projection, Offset(eye, forward, kAheadDistance),
                             display.x, display.y),
      .probe = WorldToScreen(
          projection,
          Offset(Offset(eye, forward, kProbeDistance), right, kProbeOffset),
          display.x, display.y)};
}

void DrawMarker(const std::optional<ScreenPoint>& point, ImU32 colour) {
  if (!point) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2{point->x, point->y},
                                                  kMarkerRadius, colour);
}

}  // namespace

void DrawFrame() noexcept {
  if (!config::Current().espEnabled) {
    return;
  }

  const ImVec2 kDisplay{ImGui::GetIO().DisplaySize};

  ImGui::SetNextWindowSize(ImVec2{kPanelWidth, kPanelHeight},
                           ImGuiCond_FirstUseEver);
  ImGui::Begin("ESP");
  ShowChain(games::fc3::ReadDiagnostics(), kDisplay);
  ImGui::SeparatorText("camera");

  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    ImGui::TextUnformatted(kCamera.error().message().c_str());
    ImGui::End();
    return;
  }
  if (kDisplay.x <= 0.0F || kDisplay.y <= 0.0F) {
    ImGui::TextUnformatted("the imgui display size is empty");
    ImGui::End();
    return;
  }

  const Vec3 kEye{ToVec3(kCamera->position)};
  const Vec3 kForward{ToVec3(kCamera->forward)};
  const Vec3 kRight{ToVec3(kCamera->right)};
  const float kAspect{kDisplay.x / kDisplay.y};
  const float kVerticalFov{VerticalFov(kCamera->fovHorizontalRadians, kAspect)};
  ShowCamera(*kCamera, kRight, kAspect, kVerticalFov);

  const Matrix4 kViewProjection{
      Multiply(BuildView(kEye, kForward, ToVec3(kCamera->upAxis), kRight),
               BuildProjection(kVerticalFov, kAspect, kNearPlane, kFarPlane))};

  const Markers kMarkers{
      ProjectMarkers(kViewProjection, kEye, kForward, kRight, kDisplay)};

  ShowMarkers(kDisplay, kMarkers.ahead, kMarkers.probe);
  ShowPools(kEye);
  ShowInspector(kEye, kForward);
  ShowVtables();
  ImGui::End();

  DrawEntities(kEye, kViewProjection, kDisplay);

  DrawMarker(kMarkers.ahead, kAheadColour);
  DrawMarker(kMarkers.probe, kRightColour);
}

}  // namespace afw::esp
