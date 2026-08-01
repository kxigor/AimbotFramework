#include <imgui.h>

#include <afw/esp/esp.hpp>
#include <afw/esp/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <cmath>
#include <cstdint>
#include <expected>
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
constexpr double kDegreesPerHalfTurn{180.0};

constexpr ImU32 kAheadColour{0xFF00FF00};
constexpr ImU32 kRightColour{0xFF0080FF};

[[nodiscard]] Vec3 ToVec3(const games::fc3::Triple& triple) noexcept {
  return Vec3{.x = triple.at(0), .y = triple.at(1), .z = triple.at(2)};
}

[[nodiscard]] Vec3 Offset(const Vec3& origin, const Vec3& direction,
                          float distance) noexcept {
  return Vec3{.x = origin.x + (direction.x * distance),
              .y = origin.y + (direction.y * distance),
              .z = origin.z + (direction.z * distance)};
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
  ImGui::Text("%-18s %.0f x %.0f  (used)", "imgui display",
              static_cast<double>(display.x), static_cast<double>(display.y));
}

void ShowCamera(const games::fc3::CameraBasis& camera, const Vec3& right,
                float aspect) {
  ShowAxis("position", ToVec3(camera.position));
  ShowAxis("forward", ToVec3(camera.forward));
  ShowAxis("up", ToVec3(camera.upAxis));
  ShowAxis("right (cross)", right);
  ImGui::Text("%-18s %.4f rad = %.1f deg", "vertical fov",
              static_cast<double>(camera.fovRadians),
              static_cast<double>(camera.fovRadians) * kDegreesPerHalfTurn /
                  std::numbers::pi);
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

void DrawMarker(const std::optional<ScreenPoint>& point, ImU32 colour) {
  if (!point) {
    return;
  }

  ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2{point->x, point->y},
                                                  kMarkerRadius, colour);
}

}  // namespace

void DrawFrame() noexcept {
  const ImVec2 kDisplay{ImGui::GetIO().DisplaySize};

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
  ShowCamera(*kCamera, kRight, kAspect);

  const Matrix4 kViewProjection{Multiply(
      BuildView(kEye, kForward, ToVec3(kCamera->upAxis), kRight),
      BuildProjection(kCamera->fovRadians, kAspect, kNearPlane, kFarPlane))};

  const std::optional<ScreenPoint> kAhead{
      WorldToScreen(kViewProjection, Offset(kEye, kForward, kAheadDistance),
                    kDisplay.x, kDisplay.y)};
  const std::optional<ScreenPoint> kProbe{WorldToScreen(
      kViewProjection,
      Offset(Offset(kEye, kForward, kProbeDistance), kRight, kProbeOffset),
      kDisplay.x, kDisplay.y)};

  ShowMarkers(kDisplay, kAhead, kProbe);
  ImGui::End();

  DrawMarker(kAhead, kAheadColour);
  DrawMarker(kProbe, kRightColour);
}

}  // namespace afw::esp
