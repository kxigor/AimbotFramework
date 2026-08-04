#include "farcry3.hpp"

#include <windows.h>

#include <afw/abi/abi.hpp>
#include <afw/aimbot/aimbot.hpp>
#include <afw/app/detour_guard.hpp>
#include <afw/app/log.hpp>
#include <afw/app/scoped_hooks.hpp>
#include <afw/config/config.hpp>
#include <afw/esp/esp.hpp>
#include <afw/esp/internal/classify.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/hook/patch.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/hook/vtable.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/vec3.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <optional>
#include <system_error>
#include <utility>

namespace afw::dll {

namespace {

constexpr std::size_t kMessageChars{256};
constexpr std::uint64_t kStatusInterval{1800};

void AFW_ENGINE_CALL RunAllGroupDetour(void* scheduler) {
  const app::DetourGuard kInside{};

  if (config::Current().espEnabled) {
    games::fc3::TakeCensus(std::bit_cast<std::uintptr_t>(scheduler));
  }

  games::fc3::RunAllGroupFn original{games::fc3::OriginalRunAllGroup()};
  if (original != nullptr) {
    original(scheduler);
  }
}

constexpr int kAimKey{VK_XBUTTON1};
constexpr float kDegreesPerRadian{57.2957795F};
constexpr float kMaxFrameSeconds{0.25F};

[[nodiscard]] bool IsHeld(int key) noexcept {
  constexpr unsigned kDownBit{0x8000U};

  return (static_cast<unsigned>(GetAsyncKeyState(key)) & kDownBit) != 0U;
}

[[nodiscard]] std::uintptr_t& CameraStateCache() noexcept {
  static std::uintptr_t state{0};
  return state;
}

[[nodiscard]] std::uintptr_t PlayerAddress(
    const games::fc3::EntitySnapshot& snapshot) noexcept {
  for (std::uint32_t index{0}; index < snapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{snapshot.items.at(index)};
    if (kEntity.pawn && kEntity.pawn->agentType == games::fc3::kAgentPlayer) {
      return kEntity.address;
    }
  }

  return 0;
}

[[nodiscard]] bool Wanted(const games::fc3::EntityInfo& entity,
                          const games::fc3::PawnInfo* self) noexcept {
  if (!entity.pawn || entity.pawn->dead) {
    return false;
  }

  const config::Settings& kSettings{config::Current()};
  switch (esp::KindOf(entity, self)) {
    case esp::Kind::Enemy:
      return kSettings.aimEnemies.load();
    case esp::Kind::Predator:
    case esp::Kind::Animal:
      return kSettings.aimAnimals.load();
    case esp::Kind::Bird:
      return kSettings.aimBirds.load();
    case esp::Kind::Vehicle:
      return kSettings.aimVehicles.load();
    case esp::Kind::Self:
    case esp::Kind::Ally:
    case esp::Kind::Civilian:
    case esp::Kind::Prop:
    default:
      return false;
  }
}

[[nodiscard]] std::optional<aimbot::Target> Pick(
    const games::fc3::EntitySnapshot& snapshot,
    const games::fc3::CameraBasis& camera) noexcept {
  const config::Settings& kSettings{config::Current()};
  const games::fc3::PawnInfo* const kSelf{esp::FindSelf(snapshot)};
  const aimbot::Limits kLimits{
      .maxDistance = kSettings.aimRange.load(),
      .maxError = kSettings.aimConeDegrees.load() / kDegreesPerRadian};

  return aimbot::ChooseTarget(snapshot, camera.position, camera.forward,
                              kLimits,
                              [kSelf](const games::fc3::EntityInfo& entity) {
                                return Wanted(entity, kSelf);
                              });
}

[[nodiscard]] std::uintptr_t CameraState(
    const games::fc3::EntitySnapshot& snapshot,
    const math::Vec3& forward) noexcept {
  if (CameraStateCache() != 0 &&
      games::fc3::ReproducesTheView(CameraStateCache(), forward)) {
    return CameraStateCache();
  }

  const std::uintptr_t kPlayer{PlayerAddress(snapshot)};
  if (kPlayer == 0) {
    return 0;
  }
  CameraStateCache() = games::fc3::FindViewAngles(kPlayer, forward).value_or(0);

  return CameraStateCache();
}

[[nodiscard]] std::optional<hook::CodePatch>& RecoilPatch() noexcept {
  static std::optional<hook::CodePatch> patch;
  return patch;
}

void SteadyAim() noexcept {
  const bool kWanted{config::Current().noRecoil.load()};
  if (kWanted == RecoilPatch().has_value()) {
    return;
  }

  if (!kWanted) {
    RecoilPatch().reset();
    return;
  }

  std::expected<hook::CodePatch, std::error_code> applied{
      games::fc3::SuppressRecoil()};
  if (applied) {
    RecoilPatch().emplace(std::move(*applied));
  }
}

[[nodiscard]] aimbot::MotionTable& Motion() noexcept {
  static aimbot::MotionTable table{};
  return table;
}

[[nodiscard]] float FrameSeconds() noexcept {
  static LARGE_INTEGER previous{};
  LARGE_INTEGER frequency{};
  LARGE_INTEGER now{};
  if (QueryPerformanceFrequency(&frequency) == FALSE ||
      QueryPerformanceCounter(&now) == FALSE || frequency.QuadPart == 0) {
    return 0.0F;
  }

  const std::int64_t kTicks{now.QuadPart - previous.QuadPart};
  previous = now;
  if (previous.QuadPart == now.QuadPart && kTicks == now.QuadPart) {
    return 0.0F;
  }

  const float kSeconds{static_cast<float>(
      static_cast<double>(kTicks) / static_cast<double>(frequency.QuadPart))};

  return (kSeconds > 0.0F && kSeconds < kMaxFrameSeconds) ? kSeconds : 0.0F;
}

[[nodiscard]] math::Vec3 WhereToShoot(const aimbot::Target& target) noexcept {
  Motion().Observe(target.entity, target.point, FrameSeconds());
  if (!config::Current().aimLead.load()) {
    return target.point;
  }

  return aimbot::Lead(
      target.point, Motion().VelocityOf(target.entity),
      aimbot::TravelTime(target.distance,
                         config::Current().aimBulletSpeed.load()));
}

void AimAt(std::uintptr_t state, const games::fc3::EntitySnapshot& snapshot,
           const games::fc3::CameraBasis& camera) noexcept {
  const std::optional<math::ViewAngles> kCurrent{
      games::fc3::ReadViewAngles(state)};
  const std::optional<aimbot::Target> kTarget{Pick(snapshot, camera)};
  if (!kCurrent || !kTarget) {
    return;
  }

  const math::ViewAngles kWanted{
      games::fc3::ViewAnglesTo(WhereToShoot(*kTarget) - camera.position)};
  const math::ViewAngles kStep{
      aimbot::Share(aimbot::Difference(*kCurrent, kWanted),
                    config::Current().aimSmoothing.load())};

  games::fc3::WriteViewAngles(
      state, math::ViewAngles{.yaw = math::WrapAngle(kCurrent->yaw + kStep.yaw),
                              .pitch = kCurrent->pitch + kStep.pitch});
}

void RunAssist() noexcept {
  const config::Settings& kSettings{config::Current()};
  if (!kSettings.aimEnabled.load() || !IsHeld(kAimKey)) {
    return;
  }

  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    return;
  }

  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const std::uintptr_t kState{CameraState(kSnapshot, kCamera->forward)};
  if (kState == 0) {
    return;
  }

  AimAt(kState, kSnapshot, *kCamera);
}

void LogStatus(std::uint64_t frames) {
  const games::fc3::Diagnostics kReport{games::fc3::Diagnostics::Read()};
  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};

  std::array<char, kMessageChars> message{};
  if (!kCamera) {
    std::snprintf(message.data(), message.size(),
                  "[aimbot] frame %llu | view %llX | %s\n", frames,
                  static_cast<std::uint64_t>(kReport.cameraView),
                  kCamera.error().message().c_str());
  } else {
    std::snprintf(
        message.data(), message.size(),
        "[aimbot] frame %llu | view %llX | pos %.1f %.1f %.1f | fov %.4f | "
        "render %dx%d\n",
        frames, static_cast<std::uint64_t>(kReport.cameraView),
        static_cast<double>(kCamera->position.x),
        static_cast<double>(kCamera->position.y),
        static_cast<double>(kCamera->position.z),
        static_cast<double>(kCamera->fovRadians), kReport.viewportWidth,
        kReport.viewportHeight);
  }

  app::Log(message.data());
}

}  // namespace

const char* FarCry3::Name() const noexcept { return "Far Cry 3"; }

void FarCry3::InstallHooks(app::ScopedHooks& hooks) {
  hooks.Add<games::fc3::UpdateBinding>(
      hook::VmtHook::CreateDynamic(hook::Vtable{games::fc3::RunAllGroupSlot()},
                                   0),
      &RunAllGroupDetour, "entity update");
}

void FarCry3::OnFrame() noexcept {
  static std::uint64_t frames{0};
  ++frames;

  games::fc3::PublishCensus();
  SteadyAim();
  RunAssist();

  if (frames % kStatusInterval == 0) {
    LogStatus(frames);
  }

  esp::DrawFrame();
}

}  // namespace afw::dll
