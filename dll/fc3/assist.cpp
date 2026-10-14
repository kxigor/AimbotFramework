#include <windows.h>

#include <afw/aimbot/aimbot.hpp>
#include <afw/config/config.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/vec3.hpp>
#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

#include "internal.hpp"

namespace afw::dll {

namespace {

constexpr int kAimKey{VK_XBUTTON1};

constexpr std::uint64_t kFireCycle{5};
constexpr std::uint64_t kFireDown{4};

[[nodiscard]] bool IsHeld(int key) noexcept {
  constexpr unsigned kDownBit{0x8000U};

  return (static_cast<unsigned>(GetAsyncKeyState(key)) & kDownBit) != 0U;
}

[[nodiscard]] bool OwnWindowHasFocus() noexcept {
  DWORD owner{0};
  if (GetWindowThreadProcessId(GetForegroundWindow(), &owner) == 0) {
    return false;
  }

  return owner == GetCurrentProcessId();
}

void HoldFire(bool wanted) noexcept {
  static bool held{false};
  static std::uint64_t phase{0};

  const bool kDown{wanted && (phase % kFireCycle) < kFireDown};
  phase = wanted ? phase + 1 : 0;
  if (kDown == held) {
    return;
  }

  INPUT input{};
  input.type = INPUT_MOUSE;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
  input.mi.dwFlags = kDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
  if (SendInput(1, &input, sizeof(input)) == 1) {
    held = kDown;
  }
}

[[nodiscard]] std::optional<float> AimAt(
    std::uintptr_t state, const math::Vec3& point,
    const games::fc3::CameraBasis& camera) noexcept {
  const std::optional<math::ViewAngles> kCurrent{
      games::fc3::ReadViewAngles(state)};
  if (!kCurrent) {
    return std::nullopt;
  }

  const math::ViewAngles kWanted{
      games::fc3::ViewAnglesTo(point - camera.position)};
  const math::ViewAngles kLeft{math::Difference(*kCurrent, kWanted)};
  const math::ViewAngles kStep{
      math::Share(kLeft, config::Current().aimSmoothing.load())};

  games::fc3::WriteViewAngles(
      state, math::ViewAngles{.yaw = math::WrapAngle(kCurrent->yaw + kStep.yaw),
                              .pitch = kCurrent->pitch + kStep.pitch});

  return std::hypot(kLeft.yaw - kStep.yaw, kLeft.pitch - kStep.pitch);
}

}  // namespace

void RunAssist() noexcept {
  const config::Settings& kSettings{config::Current()};
  if (!kSettings.aimEnabled.load() || !IsHeld(kAimKey)) {
    HoldFire(false);
    return;
  }

  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    HoldFire(false);
    return;
  }

  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const std::uintptr_t kState{CameraState(kSnapshot, kCamera->forward)};
  const std::optional<aimbot::Target> kTarget{Pick(kSnapshot, *kCamera)};
  if (kState == 0 || !kTarget) {
    HoldFire(false);
    return;
  }

  const std::optional<float> kRest{AimAt(kState, kTarget->point, *kCamera)};

  HoldFire(kSettings.autoFire.load() && OwnWindowHasFocus() && kRest &&
           *kRest <= math::HalfAngle(kSettings.autoFireCone.load()));
}

}  // namespace afw::dll
