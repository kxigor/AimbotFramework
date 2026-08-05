#include <imgui.h>

#include <afw/esp/internal/panel.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/render/overlay.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <optional>

namespace afw::esp {

namespace {

constexpr double kHalf{0.5};
constexpr double kDegreesPerHalfTurn{180.0};

constexpr std::array<const char*, games::fc3::kRejectionCount> kRejectionNames{
    "no vtable",  "not rigid",        "bad box",
    "not finite", "beyond the world", "template spawn"};

void ShowAngle(const char* label, float radians) {
  ImGui::Text(
      "%-18s %.4f rad = %.2f deg", label, static_cast<double>(radians),
      static_cast<double>(radians) * kDegreesPerHalfTurn / std::numbers::pi);
}

void ShowCensus() {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  ImGui::Text("%-18s %u", "in the world", kSnapshot.count);
  ImGui::Text("%-18s %u", "over capacity", kSnapshot.dropped);
  ImGui::Text("%-18s %u", "humanoid boxes", kSnapshot.humanoids);
  ImGui::Text("%-18s %u", "pawns", kSnapshot.pawns);
  ImGui::Text("%-18s %u", "via component", kSnapshot.viaComponent);
  ImGui::Text("%-18s %u hit  %u query", "region cache", kSnapshot.regionHits,
              kSnapshot.regionQueries);

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

}  // namespace

void ShowAddress(const char* label, std::uintptr_t value) {
  if (value == 0) {
    ImGui::Text("%-18s --", label);
    return;
  }
  ImGui::Text("%-18s %llX", label, static_cast<std::uint64_t>(value));
}

void ShowAxis(const char* label, const math::Vec3& axis) {
  ImGui::Text("%-18s %8.3f %8.3f %8.3f  |%.3f|", label,
              static_cast<double>(axis.x), static_cast<double>(axis.y),
              static_cast<double>(axis.z), static_cast<double>(axis.Length()));
}

void ShowVector(const char* label, const math::Vec3& value) {
  ImGui::Text("%-14s %9.3f %9.3f %9.3f", label, static_cast<double>(value.x),
              static_cast<double>(value.y), static_cast<double>(value.z));
}

void ShowPoint(const char* label,
               const std::optional<math::ScreenPoint>& point) {
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

  ShowCensus();
}

void ShowCamera(const games::fc3::CameraBasis& camera, const math::Vec3& right,
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

void ShowMarkers(const ImVec2& display,
                 const std::optional<math::ScreenPoint>& ahead,
                 const std::optional<math::ScreenPoint>& probe) {
  ImGui::SeparatorText("markers");
  ImGui::Text("%-18s %8.1f %8.1f", "screen centre",
              static_cast<double>(display.x) * kHalf,
              static_cast<double>(display.y) * kHalf);
  ShowPoint("ahead 2m", ahead);
  ShowPoint("right 2m at 10m", probe);
}

}  // namespace afw::esp
