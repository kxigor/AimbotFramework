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

// Scaffolding: chain diagnostics, the camera's own numbers, pool and entity
// counts. Everything the overlay is actually for is drawn without it, which is
// why it is off unless asked for.
void ShowPanel(const games::fc3::CameraBasis& camera, const ImVec2& display,
               const Markers& markers) {
  const float kAspect{display.x / display.y};

  ImGui::SetNextWindowSize(ImVec2{kPanelWidth, kPanelHeight},
                           ImGuiCond_FirstUseEver);
  ImGui::Begin("ESP");
  ShowChain(games::fc3::Diagnostics::Read(), display);
  ImGui::SeparatorText("camera");
  ShowCamera(camera, camera.right, kAspect,
             VerticalFov(camera.fovHorizontalRadians, kAspect));
  ShowPanelBody(camera.position, camera.forward, display, markers);
  ImGui::End();
}

void ShowNoCamera(const char* reason) {
  ImGui::Begin("ESP");
  ImGui::TextUnformatted(reason);
  ImGui::End();
}

void DrawFrame() noexcept {
  if (!config::Current().espEnabled) {
    return;
  }

  const bool kPanel{config::Current().espPanel.load()};
  const ImVec2 kDisplay{ImGui::GetIO().DisplaySize};
  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    if (kPanel) {
      ShowNoCamera(kCamera.error().message().c_str());
    }
    return;
  }
  if (kDisplay.x <= 0.0F || kDisplay.y <= 0.0F) {
    if (kPanel) {
      ShowNoCamera("the imgui display size is empty");
    }
    return;
  }

  const float kAspect{kDisplay.x / kDisplay.y};
  const float kVerticalFov{VerticalFov(kCamera->fovHorizontalRadians, kAspect)};
  const Matrix4 kViewProjection{
      Multiply(BuildView(kCamera->position, kCamera->forward, kCamera->upAxis,
                         kCamera->right),
               BuildProjection(kVerticalFov, kAspect, kNearPlane, kFarPlane))};
  const Markers kMarkers{ProjectMarkers(kViewProjection, kCamera->position,
                                        kCamera->forward, kCamera->right,
                                        kDisplay)};

  if (kPanel) {
    ShowPanel(*kCamera, kDisplay, kMarkers);
  }

  DrawEntities(kCamera->position, kViewProjection, kDisplay);
  DrawAimCone(kDisplay, kVerticalFov);
  DrawCrosshair(kMarkers.ahead, kAheadColour);
}

}  // namespace afw::esp
