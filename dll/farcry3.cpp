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

constexpr std::uint64_t kTraceInterval{60};
constexpr float kTraceReach{100.0F};

constexpr std::uint64_t kTraceWait{20};

void LogAnswer(int query, const games::fc3::TraceAnswer& answer) {
  std::array<char, kMessageChars> message{};
  if (answer.hits == 0) {
    std::snprintf(message.data(), message.size(),
                  "[aimbot] trace %d: clear, nothing in the way\n", query);
  } else {
    std::snprintf(message.data(), message.size(),
                  "[aimbot] trace %d: %u hit(s), first %.2f %.2f %.2f\n", query,
                  answer.hits, static_cast<double>(answer.first.point.x),
                  static_cast<double>(answer.first.point.y),
                  static_cast<double>(answer.first.point.z));
  }

  app::Log(message.data());
}

void ProbeTrace() {
  static std::uint64_t frames{0};
  static std::optional<int> pending;
  static std::uint64_t waited{0};

  ++frames;
  if (!config::Current().traceProbe.load()) {
    pending.reset();
    return;
  }

  if (pending) {
    const std::optional<games::fc3::TraceAnswer> kAnswer{
        games::fc3::TraceLook(*pending)};
    ++waited;
    if (kAnswer) {
      LogAnswer(*pending, *kAnswer);
      pending.reset();
      return;
    }
    if (waited < kTraceWait) {
      return;
    }

    std::array<char, kMessageChars> message{};
    std::snprintf(message.data(), message.size(),
                  "[aimbot] trace %d: never filed after %llu frames\n",
                  *pending, kTraceWait);
    app::Log(message.data());
    pending.reset();
    return;
  }
  if (frames % kTraceInterval != 0) {
    return;
  }

  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    return;
  }

  pending = games::fc3::TraceSegment(
      kCamera->position, kCamera->position + (kCamera->forward * kTraceReach));
  waited = 0;
  if (!pending) {
    app::Log("[aimbot] trace: no collision world\n");
  }
}

void UpdateReach() noexcept;

void AFW_ENGINE_CALL RunAllGroupDetour(void* scheduler) {
  const app::DetourGuard kInside{};

  if (config::Current().espEnabled) {
    games::fc3::TakeCensus(std::bit_cast<std::uintptr_t>(scheduler));
  }
  ProbeTrace();
  UpdateReach();

  games::fc3::RunAllGroupFn original{games::fc3::OriginalRunAllGroup()};
  if (original != nullptr) {
    original(scheduler);
  }
}

constexpr int kAimKey{VK_XBUTTON1};
constexpr float kDegreesPerRadian{57.2957795F};

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

[[nodiscard]] float HalfConeRadians(float degrees) noexcept {
  constexpr float kHalf{0.5F};

  return degrees * kHalf / kDegreesPerRadian;
}

[[nodiscard]] std::optional<float> Ranked(bool wanted,
                                          float priority) noexcept {
  return wanted ? std::optional<float>{priority} : std::nullopt;
}

[[nodiscard]] std::optional<float> PriorityOf(
    const games::fc3::EntityInfo& entity,
    const games::fc3::PawnInfo* self) noexcept {
  if (!entity.pawn || entity.pawn->dead) {
    return std::nullopt;
  }

  const config::Settings& kSettings{config::Current()};
  switch (esp::KindOf(entity, self)) {
    case esp::Kind::Enemy:
      return Ranked(kSettings.aimEnemies.load(),
                    kSettings.aimPriorityEnemies.load());
    case esp::Kind::Predator:
    case esp::Kind::Animal:
      return Ranked(kSettings.aimAnimals.load(),
                    kSettings.aimPriorityAnimals.load());
    case esp::Kind::Bird:
      return Ranked(kSettings.aimBirds.load(),
                    kSettings.aimPriorityBirds.load());
    case esp::Kind::Vehicle:
      return Ranked(kSettings.aimVehicles.load(),
                    kSettings.aimPriorityVehicles.load());
    case esp::Kind::Self:
    case esp::Kind::Ally:
    case esp::Kind::Civilian:
    case esp::Kind::Prop:
    default:
      return std::nullopt;
  }
}

void UpdateReach() noexcept {
  static std::uint64_t frames{0};
  ++frames;

  if (config::Current().aimWeightReach.load() <= 0.0F) {
    return;
  }

  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};
  if (!kCamera) {
    return;
  }

  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{esp::FindSelf(kSnapshot)};
  const float kRange{config::Current().aimRange.load()};
  const float kCone{HalfConeRadians(config::Current().aimConeDegrees.load())};

  for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{kSnapshot.items.at(index)};
    if (!PriorityOf(kEntity, kSelf)) {
      continue;
    }

    const math::Vec3 kPoint{aimbot::AimPoint(kEntity)};
    const math::Vec3 kOffset{kPoint - kCamera->position};
    if (kOffset.Length() > kRange ||
        aimbot::AngleBetween(kCamera->forward, kOffset) > kCone) {
      continue;
    }

    games::fc3::ObserveReach(kEntity.address, kCamera->position, kPoint,
                             frames);
  }
}

[[nodiscard]] std::optional<aimbot::Target> Pick(
    const games::fc3::EntitySnapshot& snapshot,
    const games::fc3::CameraBasis& camera) noexcept {
  const config::Settings& kSettings{config::Current()};
  const games::fc3::PawnInfo* const kSelf{esp::FindSelf(snapshot)};
  const aimbot::Choice kChoice{
      .limits = {.maxDistance = kSettings.aimRange.load(),
                 .maxError = HalfConeRadians(kSettings.aimConeDegrees.load())},
      .weights = {.aim = kSettings.aimWeightAim.load(),
                  .distance = kSettings.aimWeightDistance.load(),
                  .priority = kSettings.aimWeightPriority.load(),
                  .reach = kSettings.aimWeightReach.load()}};

  return aimbot::ChooseTarget(
      snapshot, camera.position, camera.forward, kChoice,
      [kSelf](const games::fc3::EntityInfo& entity) {
        const std::optional<float> kPriority{PriorityOf(entity, kSelf)};
        if (!kPriority) {
          return std::optional<aimbot::Ranking>{};
        }

        return std::optional<aimbot::Ranking>{
            aimbot::Ranking{.priority = *kPriority,
                            .reach = games::fc3::ReachOf(entity.address)}};
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

[[nodiscard]] std::optional<hook::CodePatch>& SpreadPatch() noexcept {
  static std::optional<hook::CodePatch> patch;
  return patch;
}

[[nodiscard]] std::optional<hook::CodePatch>& SwayPatch() noexcept {
  static std::optional<hook::CodePatch> patch;
  return patch;
}

[[nodiscard]] std::optional<hook::CodePatch>& AmmoPatch() noexcept {
  static std::optional<hook::CodePatch> patch;
  return patch;
}

[[nodiscard]] std::optional<hook::CodePatch>& MagazinePatch() noexcept {
  static std::optional<hook::CodePatch> patch;
  return patch;
}

using PatchSource =
    std::expected<hook::CodePatch, std::error_code> (*)() noexcept;

void HoldPatch(std::optional<hook::CodePatch>& patch, bool wanted,
               PatchSource source) noexcept {
  if (wanted == patch.has_value()) {
    return;
  }

  if (!wanted) {
    patch.reset();
    return;
  }

  std::expected<hook::CodePatch, std::error_code> applied{source()};
  if (applied) {
    patch.emplace(std::move(*applied));
  }
}

void SteadyAim() noexcept {
  const config::Settings& kSettings{config::Current()};
  HoldPatch(RecoilPatch(), kSettings.noRecoil.load(),
            &games::fc3::SuppressRecoil);
  HoldPatch(SpreadPatch(), kSettings.noSpread.load(),
            &games::fc3::SuppressSpread);
  HoldPatch(SwayPatch(), kSettings.noSway.load(), &games::fc3::SuppressSway);
  HoldPatch(MagazinePatch(), kSettings.infiniteAmmo.load(),
            &games::fc3::IgnoreSpentRounds);
  HoldPatch(AmmoPatch(), kSettings.infiniteAmmo.load(),
            &games::fc3::IgnoreEmptyMagazine);
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
  const math::ViewAngles kLeft{aimbot::Difference(*kCurrent, kWanted)};
  const math::ViewAngles kStep{
      aimbot::Share(kLeft, config::Current().aimSmoothing.load())};

  games::fc3::WriteViewAngles(
      state, math::ViewAngles{.yaw = math::WrapAngle(kCurrent->yaw + kStep.yaw),
                              .pitch = kCurrent->pitch + kStep.pitch});

  return std::hypot(kLeft.yaw - kStep.yaw, kLeft.pitch - kStep.pitch);
}

[[nodiscard]] bool OwnWindowHasFocus() noexcept {
  DWORD owner{0};
  if (GetWindowThreadProcessId(GetForegroundWindow(), &owner) == 0) {
    return false;
  }

  return owner == GetCurrentProcessId();
}

constexpr std::uint64_t kFireCycle{5};
constexpr std::uint64_t kFireDown{4};

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
           *kRest <= HalfConeRadians(kSettings.autoFireCone.load()));
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
