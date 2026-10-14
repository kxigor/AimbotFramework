#include "farcry3.hpp"

#include <afw/abi/abi.hpp>
#include <afw/app/detour_guard.hpp>
#include <afw/app/scoped_hooks.hpp>
#include <afw/config/config.hpp>
#include <afw/esp/esp.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/hook/vtable.hpp>
#include <afw/process/memory.hpp>
#include <bit>
#include <cstdint>

#include "internal.hpp"

namespace afw::dll {

namespace {

constexpr std::uint64_t kStatusInterval{1800};

void AFW_ENGINE_CALL RunAllGroupDetour(void* scheduler) {
  const app::DetourGuard kInside{};

  if (config::Current().espEnabled.load()) {
    games::fc3::TakeCensus(std::bit_cast<std::uintptr_t>(scheduler));
  }
  ProbeTrace();
  UpdateReach();

  games::fc3::RunAllGroupFn original{games::fc3::OriginalRunAllGroup()};
  if (original != nullptr) {
    original(scheduler);
  }
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

  process::ForgetReadableRegions();
  games::fc3::PublishCensus();
  SteadyAim();
  RunAssist();

  if (frames % kStatusInterval == 0) {
    LogStatus(frames);
  }

  esp::DrawFrame();
}

}  // namespace afw::dll
