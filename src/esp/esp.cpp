#include <imgui.h>

#include <afw/config/config.hpp>
#include <afw/esp/esp.hpp>
#include <afw/esp/internal/draw.hpp>
#include <afw/esp/internal/panel.hpp>
#include <afw/esp/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::esp {

namespace {

constexpr float kNearPlane{0.1F};
constexpr float kFarPlane{10000.0F};
constexpr float kAheadDistance{2.0F};
constexpr float kProbeDistance{10.0F};
constexpr float kProbeOffset{2.0F};
constexpr float kPanelWidth{540.0F};
constexpr float kPanelHeight{1180.0F};

constexpr ImU32 kAheadColour{0xFF00FF00};
constexpr ImU32 kRightColour{0xFF0080FF};

// Two fixed points in front of the camera. They are the only way to tell a
// wrong projection from a wrong entity position: whatever the world does, these
// two must sit at the screen centre and a hand's width to its right.
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

void ShowPanelBody(const Vec3& eye, const Vec3& forward, const ImVec2& display,
                   const Markers& markers) {
  ShowMarkers(display, markers.ahead, markers.probe);
  ShowPools(eye);
  ShowEntities(eye, forward);
  ShowVtables();
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

  ShowPanelBody(kEye, kForward, kDisplay, kMarkers);
  ImGui::End();

  DrawEntities(kEye, kViewProjection, kDisplay);

  DrawAimCone(kDisplay, kVerticalFov);
  DrawCrosshair(kMarkers.ahead, kAheadColour);
  DrawMarker(kMarkers.probe, kRightColour);
}

}  // namespace afw::esp
