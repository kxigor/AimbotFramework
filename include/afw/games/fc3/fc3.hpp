#pragma once

#include <afw/abi/abi.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <afw/hook/hook.hpp>
#include <afw/hook/patch.hpp>
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

// The same agreement asked again of a state found earlier. Dying frees the
// entity the camera hung off, and the floats left behind at that address stay
// readable and finite, so a check that only reads them keeps a dead object
// forever and the view stops answering with nothing to say why.
[[nodiscard]] bool ReproducesTheView(std::uintptr_t state,
                                     const Vec3& forward) noexcept;

[[nodiscard]] std::optional<ViewAngles> ReadViewAngles(
    std::uintptr_t state) noexcept;

// Writes every copy the engine keeps. Setting only the one it commits to leaves
// the next frame reading the old heading back out of the other.
bool WriteViewAngles(std::uintptr_t state, const ViewAngles& angles) noexcept;

// Stops the shot handler storing the kick it worked out. Held for as long as
// the returned patch is alive; letting it go puts the game's own bytes back.
[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
SuppressRecoil() noexcept;

// Stops the shot direction bending by the weapon's current spread. Shotguns
// spread by a different mechanism and keep theirs. Held for as long as the
// returned patch is alive; letting it go puts the game's own bytes back.
[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
SuppressSpread() noexcept;

// Stops breathing drifting the view. Only a scoped weapon sways the camera at
// all, so this changes nothing in anyone else's hands. Held for as long as the
// returned patch is alive; letting it go puts the game's own bytes back.
[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
SuppressSway() noexcept;

// Puts a segment to the engine's own collision world and hands back the id it
// filed the walk under. Absent when the world is not up, and on any build that
// is not the game's own 32-bit one, where the call cannot be made at all.
//
// This is the one thing here that calls into the engine rather than reading it.
[[nodiscard]] std::optional<int> TraceSegment(const Vec3& from,
                                              const Vec3& target) noexcept;

// Only the point is read. A hit record carries more -- the shot handler
// resolves an entity out of it and asks which bone was struck -- but those
// fields came back the same on a warrior, a car and a hillside, so whatever
// they are, this has not identified them and will not pretend to.
struct TraceHit {
  Vec3 point{};
};

// A walk that has been done, and the nearest thing it ran into. No hits at all
// is the useful answer rather than a failure: it means the segment reached the
// far end without meeting anything, which is the whole question being asked.
// `first` says nothing when `hits` is zero.
struct TraceAnswer {
  std::uint32_t hits{};
  TraceHit first{};
};

// What that id found. Reading it is all memory and no call. Nothing at all
// comes back while the walk is still outstanding -- the engine defers it, so an
// answer takes a few frames rather than the next one -- and nothing comes back
// once the record has been recycled either.
[[nodiscard]] std::optional<TraceAnswer> TraceLook(int query) noexcept;

// Keeps one standing answer per entity to whether a shot from `eye` would get
// to `point`, asking the engine again whenever the last answer has gone stale.
// Must be called from the thread the game runs its own queries on.
void ObserveReach(std::uintptr_t entity, const Vec3& eye, const Vec3& point,
                  std::uint64_t frame) noexcept;

// The last answer, and 1 for anything never asked about. Optimistic on purpose:
// a target nothing is known about yet is aimed at rather than passed over, and
// a target is only written off once the engine has actually said so.
[[nodiscard]] float ReachOf(std::uintptr_t entity) noexcept;

[[nodiscard]] std::expected<Viewport, std::error_code> ReadViewport() noexcept;

}  // namespace afw::games::fc3
