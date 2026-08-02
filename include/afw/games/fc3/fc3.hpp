#pragma once

#include <afw/hook/vmt_hook.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>

// The engine's methods are __thiscall: on x86 `this` arrives in ECX, which a
// free function can only receive through __fastcall plus an unused second
// parameter. On x64 there is no such distinction and the attribute would be
// diagnosed as ignored.
#ifdef __i386__
#define AFW_ENGINE_CALL __attribute__((fastcall))
#else
#define AFW_ENGINE_CALL
#endif

namespace afw::games::fc3 {

constexpr std::size_t kAxisCount{3};

using Triple = std::array<float, kAxisCount>;

struct CameraBasis {
  Triple position{};
  Triple forward{};
  Triple upAxis{};
  Triple right{};
  float fovRadians{};
  float fovHorizontalRadians{};
};

struct Viewport {
  float width{};
  float height{};
};

struct Diagnostics {
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

[[nodiscard]] Diagnostics ReadDiagnostics() noexcept;

using RunAllGroupFn = void(AFW_ENGINE_CALL*)(void* self, void* unused);

// RunAllGroup runs once per update pass, so the census accumulates across calls
// and is published on the frame boundary.
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
  explicit UpdateBinding(const hook::VmtHook& hook) noexcept;

  UpdateBinding(const UpdateBinding&) = delete;
  UpdateBinding& operator=(const UpdateBinding&) = delete;

  UpdateBinding(UpdateBinding&&) = delete;
  UpdateBinding& operator=(UpdateBinding&&) = delete;

  ~UpdateBinding();
};

[[nodiscard]] RunAllGroupFn OriginalRunAllGroup() noexcept;

constexpr std::size_t kMaxEntities{4096};

struct EntityInfo {
  std::uintptr_t address{};
  std::uintptr_t vtable{};
  Triple position{};
  // The object's own axes in world space; the box at +0x6C is in that space.
  Triple axisX{};
  Triple axisY{};
  Triple axisZ{};
  Triple aabbMin{};
  Triple aabbMax{};
  float height{};
  std::int32_t army{};
  std::int32_t faction{};
  std::uint8_t ownArmyBit{};
  std::uint8_t hostileMask{};
  std::uint8_t reverseHostileMask{};
  std::uint8_t deadByte{};
  bool pawn{};
  bool humanoid{};
  bool dead{};
};

// Filled on the update thread, swapped in on the frame boundary.
struct EntitySnapshot {
  std::uint32_t count{};
  std::uint32_t dropped{};
  std::uint32_t pawns{};
  std::uint32_t humanoids{};
  std::uint32_t notEntities{};
  std::array<EntityInfo, kMaxEntities> items{};
};

[[nodiscard]] const EntitySnapshot& LastSnapshot() noexcept;

[[nodiscard]] std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept;

[[nodiscard]] std::expected<CameraBasis, std::error_code> ReadCamera() noexcept;

[[nodiscard]] std::expected<Viewport, std::error_code> ReadViewport() noexcept;

}  // namespace afw::games::fc3
