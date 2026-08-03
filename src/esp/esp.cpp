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
constexpr float kCrosshairArm{11.0F};
constexpr float kCrosshairThickness{1.5F};
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
constexpr ImU32 kEnemyColour{0xFF3030FF};
constexpr ImU32 kAllyColour{0xFF30FF30};

constexpr std::array<const char*, games::fc3::kRejectionCount> kRejectionNames{
    "no vtable",  "not rigid",        "bad box",
    "not finite", "beyond the world", "template spawn"};

constexpr ImU32 kCivilianColour{0xFFA0FFA0};
constexpr ImU32 kAnimalColour{0xFF7070A0};
constexpr ImU32 kPredatorColour{0xFF2060FF};
constexpr ImU32 kVehicleColour{0xFF40C0FF};

constexpr float kDrawDistance{150.0F};
constexpr float kPropRadius{2.0F};
constexpr float kBoxThickness{1.5F};
constexpr float kVehicleThickness{2.5F};
constexpr float kLabelLift{14.0F};
constexpr std::size_t kLabelChars{64};
constexpr std::size_t kVtableSlots{12};
constexpr float kAimTolerance{0.95F};
constexpr std::size_t kNearbyRows{8};
constexpr float kNearbyRange{25.0F};
constexpr std::int32_t kNoPawnField{-99};
constexpr float kPanelWidth{540.0F};
constexpr float kPanelHeight{1180.0F};

[[nodiscard]] float VerticalFov(float horizontalRadians,
                                float aspect) noexcept {
  return kFullAngle * std::atan(std::tan(horizontalRadians * kHalfF) / aspect);
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
              static_cast<double>(axis.z), static_cast<double>(axis.Length()));
}

void ShowPoint(const char* label, const std::optional<ScreenPoint>& point) {
  if (!point) {
    ImGui::Text("%-18s behind the camera", label);
    return;
  }
  ImGui::Text("%-18s %8.1f %8.1f", label, static_cast<double>(point->x),
              static_cast<double>(point->y));
}

[[nodiscard]] std::uint32_t CountWithArchetype(
    const games::fc3::EntitySnapshot& snapshot) noexcept {
  std::uint32_t found{0};
  for (std::uint32_t index{0}; index < snapshot.count; ++index) {
    if (snapshot.items.at(index).identity) {
      ++found;
    }
  }

  return found;
}

void ShowCensus() {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  ImGui::Text("%-18s %u", "in the world", kSnapshot.count);
  ImGui::Text("%-18s %u", "with archetype", CountWithArchetype(kSnapshot));
  ImGui::Text("%-18s %u", "over capacity", kSnapshot.dropped);
  ImGui::Text("%-18s %u", "humanoid boxes", kSnapshot.humanoids);
  ImGui::Text("%-18s %u", "pawns", kSnapshot.pawns);
  ImGui::Text("%-18s %u", "via component", kSnapshot.viaComponent);

  ImGui::SeparatorText("dropped, by reason");
  ImGui::Text("%-18s %u", "unreadable", kSnapshot.unreadable);
  ImGui::Text("%-18s %u", "not an entity", kSnapshot.foreignClass);
  for (std::size_t reason{0}; reason < games::fc3::kRejectionCount; ++reason) {
    ImGui::Text("%-18s %u", kRejectionNames.at(reason),
                kSnapshot.rejected.at(reason));
  }

  ImGui::SeparatorText("by update-group type");
  for (std::size_t type{0}; type < games::fc3::kGroupTypes; ++type) {
    const std::uint32_t kTaken{kSnapshot.acceptedByType.at(type)};
    const std::uint32_t kLost{kSnapshot.droppedByType.at(type)};
    if (kTaken == 0 && kLost == 0) {
      continue;
    }
    ImGui::Text("type %-12zu %5u kept   %5u dropped", type, kTaken, kLost);
  }
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

  ShowCensus();
}

void ShowAngle(const char* label, float radians) {
  ImGui::Text(
      "%-18s %.4f rad = %.2f deg", label, static_cast<double>(radians),
      static_cast<double>(radians) * kDegreesPerHalfTurn / std::numbers::pi);
}

void ShowCamera(const games::fc3::CameraBasis& camera, const Vec3& right,
                float aspect, float verticalFov) {
  ShowAxis("position", camera.position);
  ShowAxis("forward", camera.forward);
  ShowAxis("up", camera.upAxis);
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

[[nodiscard]] float DistanceTo(const Vec3& eye, const Vec3& point) noexcept {
  return (point - eye).Length();
}

struct ScreenRect {
  float left{};
  float top{};
  float right{};
  float bottom{};
};

[[nodiscard]] std::optional<ScreenRect> ProjectBox(
    const games::fc3::EntityInfo& entity, const Matrix4& projection,
    const ImVec2& display) noexcept {
  ScreenRect rect{.left = std::numeric_limits<float>::max(),
                  .top = std::numeric_limits<float>::max(),
                  .right = std::numeric_limits<float>::lowest(),
                  .bottom = std::numeric_limits<float>::lowest()};

  for (std::size_t corner{0}; corner < math::kAabbCorners; ++corner) {
    const std::optional<ScreenPoint> kPoint{WorldToScreen(
        projection, entity.transform.TransformPoint(entity.box.Corner(corner)),
        display.x, display.y)};
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

enum class Kind : std::uint8_t {
  Self,
  Enemy,
  Ally,
  Civilian,
  Predator,
  Animal,
  Vehicle,
  Prop
};

[[nodiscard]] bool ArmiesHostile(const games::fc3::PawnInfo& self,
                                 const games::fc3::PawnInfo& other) noexcept {
  return ((other.ownArmyBit & self.hostileMask) != 0) ||
         ((other.reverseHostileMask & self.ownArmyBit) != 0);
}

[[nodiscard]] const games::fc3::PawnInfo* FindSelf(
    const games::fc3::EntitySnapshot& snapshot) noexcept {
  for (std::uint32_t index{0}; index < snapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{snapshot.items.at(index)};
    if (kEntity.pawn && kEntity.pawn->agentType == games::fc3::kAgentPlayer) {
      return &*kEntity.pawn;
    }
  }

  return nullptr;
}

[[nodiscard]] Kind KindOfPawn(const games::fc3::PawnInfo& pawn,
                              const games::fc3::PawnInfo* self) noexcept {
  if (pawn.kind == games::fc3::AgentKind::Vehicle) {
    return Kind::Vehicle;
  }
  if (pawn.kind == games::fc3::AgentKind::SimpleAnimal) {
    return Kind::Animal;
  }
  if (pawn.kind == games::fc3::AgentKind::Animal) {
    return (self != nullptr && ArmiesHostile(*self, pawn)) ? Kind::Predator
                                                           : Kind::Animal;
  }
  if (pawn.agentType == games::fc3::kAgentPlayer) {
    return Kind::Self;
  }
  if (pawn.agentType == games::fc3::kAgentCompanion) {
    return Kind::Ally;
  }
  if (pawn.agentType == games::fc3::kAgentCivilian) {
    return Kind::Civilian;
  }
  if (pawn.faction == games::fc3::kFactionPirate ||
      pawn.faction == games::fc3::kFactionPrivateer) {
    return Kind::Enemy;
  }
  if (pawn.faction == games::fc3::kFactionRebel) {
    return Kind::Ally;
  }
  return (self != nullptr && ArmiesHostile(*self, pawn)) ? Kind::Enemy
                                                         : Kind::Civilian;
}

[[nodiscard]] Kind KindOf(const games::fc3::EntityInfo& entity,
                          const games::fc3::PawnInfo* self) noexcept {
  if (entity.pawn) {
    return KindOfPawn(*entity.pawn, self);
  }

  return Kind::Prop;
}

[[nodiscard]] ImU32 ColourOf(Kind kind) noexcept {
  switch (kind) {
    case Kind::Self:
    case Kind::Enemy:
      return kEnemyColour;
    case Kind::Ally:
      return kAllyColour;
    case Kind::Civilian:
      return kCivilianColour;
    case Kind::Predator:
      return kPredatorColour;
    case Kind::Animal:
      return kAnimalColour;
    case Kind::Vehicle:
      return kVehicleColour;
    case Kind::Prop:
    default:
      return kPropColour;
  }
}

void DrawLabel(const games::fc3::EntityInfo& entity, const ScreenRect& rect,
               ImU32 colour) {
  std::array<char, kLabelChars> label{};
  if (entity.identity) {
    std::snprintf(label.data(), label.size(), "%08X h%.1f",
                  entity.identity->classHash,
                  static_cast<double>(entity.box.Extent().z));
  } else {
    std::snprintf(label.data(), label.size(), "-------- h%.1f",
                  static_cast<double>(entity.box.Extent().z));
  }
  ImGui::GetBackgroundDrawList()->AddText(
      ImVec2{rect.left, rect.top - kLabelLift}, colour, label.data());
}

void DrawBox(const games::fc3::EntityInfo& entity, const Matrix4& projection,
             const ImVec2& display, ImU32 colour, float thickness) {
  const std::optional<ScreenRect> kRect{
      ProjectBox(entity, projection, display)};
  if (!kRect) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddRect(ImVec2{kRect->left, kRect->top},
                                          ImVec2{kRect->right, kRect->bottom},
                                          colour, 0.0F, 0, thickness);
  DrawLabel(entity, *kRect, colour);
}

void DrawRing(const games::fc3::EntityInfo& entity, const Matrix4& projection,
              const ImVec2& display, ImU32 colour) {
  const std::optional<ScreenRect> kRect{
      ProjectBox(entity, projection, display)};
  if (!kRect) {
    return;
  }

  const ImVec2 kCentre{(kRect->left + kRect->right) * kHalfF,
                       (kRect->top + kRect->bottom) * kHalfF};
  const float kRadius{(kRect->bottom - kRect->top) * kHalfF};
  ImGui::GetBackgroundDrawList()->AddCircle(kCentre, kRadius, colour, 0,
                                            kBoxThickness);
  DrawLabel(entity, *kRect, colour);
}

void DrawEntities(const Vec3& eye, const Matrix4& projection,
                  const ImVec2& display) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{FindSelf(kSnapshot)};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    if (DistanceTo(eye, kEntity.transform.Translation()) > kDrawDistance) {
      continue;
    }

    const Kind kKind{KindOf(kEntity, kSelf)};
    if (kKind == Kind::Self) {
      continue;
    }

    const ImU32 kColour{ColourOf(kKind)};

    if (kKind == Kind::Animal) {
      DrawRing(kEntity, projection, display, kColour);
      continue;
    }
    if (kKind != Kind::Prop) {
      DrawBox(kEntity, projection, display, kColour,
              kKind == Kind::Vehicle ? kVehicleThickness : kBoxThickness);
      continue;
    }

    const std::optional<ScreenPoint> kPoint{WorldToScreen(
        projection, kEntity.transform.Translation(), display.x, display.y)};
    if (kPoint) {
      ImGui::GetBackgroundDrawList()->AddCircleFilled(
          ImVec2{kPoint->x, kPoint->y}, kPropRadius, kColour);
    }
  }
}

void ShowVector(const char* label, const Vec3& value) {
  ImGui::Text("%-14s %9.3f %9.3f %9.3f", label, static_cast<double>(value.x),
              static_cast<double>(value.y), static_cast<double>(value.z));
}

struct AimHit {
  const games::fc3::EntityInfo* entity{nullptr};
  float distance{0.0F};
};

[[nodiscard]] AimHit UnderCrosshair(const Vec3& eye,
                                    const Vec3& forward) noexcept {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};

  AimHit hit{};
  AimHit spare{};
  float best{kAimTolerance};
  float fallback{kAimTolerance};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    const Vec3 kDelta{kEntity.transform.Translation() - eye};
    const float kDistance{kDelta.Length()};
    if (kDistance < 1.0F || kDistance > kDrawDistance) {
      continue;
    }

    const Vec3 kDirection{kDelta.Normalised()};
    const float kDot{kDirection.Dot(forward)};
    if (kEntity.pawn) {
      if (kDot > best) {
        best = kDot;
        hit = AimHit{.entity = &kEntity, .distance = kDistance};
      }
      continue;
    }
    if (hit.entity == nullptr && kDot > fallback) {
      fallback = kDot;
      spare = AimHit{.entity = &kEntity, .distance = kDistance};
    }
  }

  return hit.entity != nullptr ? hit : spare;
}

struct NearbyRow {
  const games::fc3::EntityInfo* entity{nullptr};
  float distance{0.0F};
};

using NearbyRows = std::array<NearbyRow, kNearbyRows>;

void Insert(NearbyRows& rows, std::size_t& used,
            const NearbyRow& row) noexcept {
  std::size_t slot{used < kNearbyRows ? used : kNearbyRows - 1};
  if (used < kNearbyRows) {
    ++used;
  } else if (rows.at(slot).distance <= row.distance) {
    return;
  }

  while (slot > 0 && rows.at(slot - 1).distance > row.distance) {
    rows.at(slot) = rows.at(slot - 1);
    --slot;
  }
  rows.at(slot) = row;
}

[[nodiscard]] const char* KindName(Kind kind) noexcept {
  switch (kind) {
    case Kind::Self:
      return "self";
    case Kind::Enemy:
      return "enemy";
    case Kind::Ally:
      return "ally";
    case Kind::Civilian:
      return "civilian";
    case Kind::Predator:
      return "predator";
    case Kind::Animal:
      return "animal";
    case Kind::Vehicle:
      return "vehicle";
    case Kind::Prop:
    default:
      return "prop";
  }
}

void ShowNearby(const Vec3& eye) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{FindSelf(kSnapshot)};
  NearbyRows rows{};
  std::size_t used{0};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    const float kDistance{DistanceTo(eye, kEntity.transform.Translation())};
    if (kDistance <= kNearbyRange) {
      Insert(rows, used, NearbyRow{.entity = &kEntity, .distance = kDistance});
    }
  }

  ImGui::SeparatorText("nearest");
  for (std::size_t index{0}; index < used; ++index) {
    const games::fc3::EntityInfo& kEntity{*rows.at(index).entity};
    ImGui::Text("%8llX %5.1fm %-8s a%-3d f%-3d %s",
                static_cast<std::uint64_t>(kEntity.address),
                static_cast<double>(rows.at(index).distance),
                KindName(KindOf(kEntity, kSelf)),
                kEntity.pawn ? kEntity.pawn->army : kNoPawnField,
                kEntity.pawn ? kEntity.pawn->faction : kNoPawnField,
                kEntity.identity ? "arch" : "--");
  }
}

void ShowInspector(const Vec3& eye, const Vec3& forward) {
  const AimHit kHit{UnderCrosshair(eye, forward)};
  const games::fc3::EntityInfo* const kNearest{kHit.entity};
  const float kBestDistance{kHit.distance};

  ImGui::SeparatorText("crosshair");
  if (kNearest == nullptr) {
    ImGui::TextUnformatted("nothing within the aim cone");
    return;
  }

  ImGui::Text("%-14s %llX", "entity",
              static_cast<std::uint64_t>(kNearest->address));
  ImGui::Text("%-14s %llX", "vtable",
              static_cast<std::uint64_t>(kNearest->vtable));
  if (kNearest->identity) {
    ImGui::Text("%-14s %u  hash %08X", "class", kNearest->identity->classIndex,
                kNearest->identity->classHash);
  } else {
    ImGui::TextUnformatted("class          none");
  }
  ImGui::Text("%-14s %.1f m", "distance", static_cast<double>(kBestDistance));
  ShowVector("position", kNearest->transform.Translation());
  ShowVector("aabb min", kNearest->box.min);
  ShowVector("aabb max", kNearest->box.max);
  if (!kNearest->pawn) {
    ImGui::TextUnformatted("not a pawn: no army fields");
    return;
  }
  const games::fc3::PawnInfo& kPawn{*kNearest->pawn};
  ImGui::Text("%-14s %d  faction %d", "army", kPawn.army, kPawn.faction);
  ImGui::Text("%-14s %u", "agent type", kPawn.agentType);
  ImGui::Text("%-14s own %02X  hostile %02X  reverse %02X", "masks",
              kPawn.ownArmyBit, kPawn.hostileMask, kPawn.reverseHostileMask);
}

void ShowEntities(const Vec3& eye, const Vec3& forward) {
  ShowInspector(eye, forward);
  ShowNearby(eye);
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
    if (DistanceTo(eye, kEntity.transform.Translation()) <= kDrawDistance) {
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
      .ahead = WorldToScreen(projection, eye + (forward * kAheadDistance),
                             display.x, display.y),
      .probe = WorldToScreen(
          projection, eye + (forward * kProbeDistance) + (right * kProbeOffset),
          display.x, display.y)};
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

}  // namespace

void DrawFrame() noexcept {
  if (!config::Current().espEnabled) {
    return;
  }

  const ImVec2 kDisplay{ImGui::GetIO().DisplaySize};

  ImGui::SetNextWindowSize(ImVec2{kPanelWidth, kPanelHeight},
                           ImGuiCond_FirstUseEver);
  ImGui::Begin("ESP");
  ShowChain(games::fc3::Diagnostics::Read(), kDisplay);
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

  const Vec3 kEye{kCamera->position};
  const Vec3 kForward{kCamera->forward};
  const Vec3 kRight{kCamera->right};
  const float kAspect{kDisplay.x / kDisplay.y};
  const float kVerticalFov{VerticalFov(kCamera->fovHorizontalRadians, kAspect)};
  ShowCamera(*kCamera, kRight, kAspect, kVerticalFov);

  const Matrix4 kViewProjection{
      Multiply(BuildView(kEye, kForward, kCamera->upAxis, kRight),
               BuildProjection(kVerticalFov, kAspect, kNearPlane, kFarPlane))};

  const Markers kMarkers{
      ProjectMarkers(kViewProjection, kEye, kForward, kRight, kDisplay)};

  ShowMarkers(kDisplay, kMarkers.ahead, kMarkers.probe);
  ShowPools(kEye);
  ShowEntities(kEye, kForward);
  ShowVtables();
  ImGui::End();

  DrawEntities(kEye, kViewProjection, kDisplay);

  DrawCrosshair(kMarkers.ahead, kAheadColour);
  DrawMarker(kMarkers.probe, kRightColour);
}

}  // namespace afw::esp
