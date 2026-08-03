#include "farcry3.hpp"

#include <afw/abi/abi.hpp>
#include <afw/app/detour_guard.hpp>
#include <afw/app/log.hpp>
#include <afw/app/scoped_hooks.hpp>
#include <afw/config/config.hpp>
#include <afw/esp/esp.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/hook/vtable.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <system_error>

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

  if (frames % kStatusInterval == 0) {
    LogStatus(frames);
  }

  esp::DrawFrame();
}

}  // namespace afw::dll
