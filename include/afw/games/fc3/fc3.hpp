#pragma once

#include <afw/abi/abi.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <afw/hook/hook.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

struct Diagnostics {
  [[nodiscard]] static Diagnostics Read() noexcept;

  std::uintptr_t moduleBase{};
  std::uintptr_t containerAddress{};
  std::uintptr_t container{};
  std::int32_t liveFlag{};
  std::uintptr_t manager{};
  std::uintptr_t cameraView{};
  std::uintptr_t viewportAddress{};
  std::uintptr_t viewportBlock{};
  std::int32_t viewportWidth{};
  std::int32_t viewportHeight{};
};

using RunAllGroupFn = void(AFW_ENGINE_CALL*)(void* scheduler);

struct EntityCensus {
  std::uint32_t frames{};
  std::uint32_t calls{};
  std::uint32_t groups{};
  std::uint32_t entities{};
  std::uint32_t skipped{};
};

[[nodiscard]] void** RunAllGroupSlot() noexcept;

void TakeCensus(std::uintptr_t scheduler) noexcept;

void PublishCensus() noexcept;

[[nodiscard]] EntityCensus LastCensus() noexcept;

class UpdateBinding {
 public:
  explicit UpdateBinding(const hook::Hook& hook) noexcept;

  UpdateBinding(const UpdateBinding&) = delete;
  UpdateBinding& operator=(const UpdateBinding&) = delete;

  UpdateBinding(UpdateBinding&&) = delete;
  UpdateBinding& operator=(UpdateBinding&&) = delete;

  ~UpdateBinding();
};

[[nodiscard]] RunAllGroupFn OriginalRunAllGroup() noexcept;

constexpr std::size_t kMaxEntities{4096};

constexpr std::size_t kRejectedSamples{8};

constexpr std::size_t kGroupTypes{32};

struct EntitySnapshot {
  void ForgetCounts() noexcept {
    count = 0;
    dropped = 0;
    pawns = 0;
    humanoids = 0;
    unreadable = 0;
    foreignClass = 0;
    viaComponent = 0;
    regionQueries = 0;
    regionHits = 0;
    rejected.fill(0);
    sampled = 0;
    notRigidSample.fill(0);
    acceptedByType.fill(0);
    droppedByType.fill(0);
  }

  void CountByType(std::uint32_t typeBit, bool accepted) noexcept {
    if (typeBit >= kGroupTypes) {
      return;
    }
    if (accepted) {
      ++acceptedByType.at(typeBit);
      return;
    }

    ++droppedByType.at(typeBit);
  }

  void SampleNotRigid(std::uintptr_t address) noexcept {
    if (sampled < kRejectedSamples) {
      notRigidSample.at(sampled) = address;
      ++sampled;
    }
  }

  std::uint32_t count{};
  std::uint32_t dropped{};
  std::uint32_t pawns{};
  std::uint32_t humanoids{};
  std::uint32_t unreadable{};
  std::uint32_t foreignClass{};
  std::uint32_t viaComponent{};
  std::uint32_t regionQueries{};
  std::uint32_t regionHits{};
  std::array<std::uint32_t, kRejectionCount> rejected{};
  std::uint32_t sampled{};
  std::array<std::uintptr_t, kRejectedSamples> notRigidSample{};
  std::array<std::uint32_t, kGroupTypes> acceptedByType{};
  std::array<std::uint32_t, kGroupTypes> droppedByType{};
  std::array<EntityInfo, kMaxEntities> items{};
};

[[nodiscard]] const EntitySnapshot& LastSnapshot() noexcept;

[[nodiscard]] std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept;

[[nodiscard]] std::expected<CameraBasis, std::error_code> ReadCamera() noexcept;

// A direction turned into the pair the engine steers by. Yaw is measured a
// quarter turn off atan2 over the world's x and y, which is why this is not
// just an arctangent at the call site.
[[nodiscard]] ViewAngles ViewAnglesTo(const Vec3& direction) noexcept;

// The camera rides on the player as a component, and the engine registers no
// class for it, so it is found by the one thing that cannot happen twice by
// accident: its stored angles reproduce the camera's own forward vector, which
// was read without going through it. What comes back is the state object, and
// the three angle blocks are offsets from there.
[[nodiscard]] std::optional<std::uintptr_t> FindViewAngles(
    std::uintptr_t playerEntity, const Vec3& forward) noexcept;

[[nodiscard]] std::optional<ViewAngles> ReadViewAngles(
    std::uintptr_t state) noexcept;

// Writes every copy the engine keeps. Setting only the one it commits to leaves
// the next frame reading the old heading back out of the other.
bool WriteViewAngles(std::uintptr_t state, const ViewAngles& angles) noexcept;

[[nodiscard]] std::expected<Viewport, std::error_code> ReadViewport() noexcept;

}  // namespace afw::games::fc3
