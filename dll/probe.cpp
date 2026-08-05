#include <afw/app/log.hpp>
#include <afw/config/config.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

#include "internal.hpp"

namespace afw::dll {

namespace {

constexpr std::uint64_t kTraceInterval{60};
constexpr float kTraceReach{100.0F};
constexpr std::uint64_t kTraceWait{20};

void LogAnswer(int query, const games::fc3::TraceAnswer& answer) {
  if (answer.hits == 0) {
    app::Logf("[aimbot] trace %d: clear, nothing in the way\n", query);
    return;
  }

  app::Logf("[aimbot] trace %d: %u hit(s), first %.2f %.2f %.2f\n", query,
            answer.hits, static_cast<double>(answer.first.point.x),
            static_cast<double>(answer.first.point.y),
            static_cast<double>(answer.first.point.z));
}

}  // namespace

// A walk is filed under an id and answered some frames later, so the probe
// keeps the id it is waiting on rather than asking and reading in one breath.
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

    app::Logf("[aimbot] trace %d: never filed after %llu frames\n", *pending,
              kTraceWait);
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

void LogStatus(std::uint64_t frames) {
  const games::fc3::Diagnostics kReport{games::fc3::Diagnostics::Read()};
  const std::expected<games::fc3::CameraBasis, std::error_code> kCamera{
      games::fc3::ReadCamera()};

  if (!kCamera) {
    app::Logf("[aimbot] frame %llu | view %llX | %s\n", frames,
              static_cast<std::uint64_t>(kReport.cameraView),
              kCamera.error().message().c_str());
    return;
  }

  app::Logf(
      "[aimbot] frame %llu | view %llX | pos %.1f %.1f %.1f | fov %.4f | "
      "render %dx%d\n",
      frames, static_cast<std::uint64_t>(kReport.cameraView),
      static_cast<double>(kCamera->position.x),
      static_cast<double>(kCamera->position.y),
      static_cast<double>(kCamera->position.z),
      static_cast<double>(kCamera->fovRadians), kReport.viewportWidth,
      kReport.viewportHeight);
}

}  // namespace afw::dll
