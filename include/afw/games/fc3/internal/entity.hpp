#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <expected>
#include <optional>

namespace afw::games::fc3 {

// Exactly what an entity's memory holds, before anything has been decided about
// it. Reading and judging are separate because the read is what fails when the
// address is not memory at all, while the judgement is a pure question about
// bytes already in hand.
struct RawEntity {
  std::uintptr_t address{};
  std::uintptr_t vtable{};
  math::Matrix4 transform{};
  math::Aabb box{};
};

[[nodiscard]] std::optional<RawEntity> ReadEntity(
    std::uintptr_t address) noexcept;

// Touches no memory: everything it decides on is in raw.
[[nodiscard]] std::expected<EntityInfo, Rejection> Classify(
    const RawEntity& raw) noexcept;

// The update groups hold objects of several classes side by side, and reading
// one class's offsets out of another yields nonsense that only looks like a
// failed entity. This asks the engine's own type registry instead, through the
// one virtual it takes to obtain a class descriptor.
[[nodiscard]] bool IsEntityClass(std::uintptr_t address) noexcept;

// Walks a component back to its entity. What comes back is confirmed to be an
// entity before it is handed out.
[[nodiscard]] std::optional<std::uintptr_t> OwnerOfComponent(
    std::uintptr_t address) noexcept;

// A fallback, not the first choice. Several components of one entity carry a
// rigid basis and they do not agree: measured across 33 characters, taking the
// first rigid one put a single pirate 118 m from where he stood. The entity's
// own matrix agreed with the truth in all 33 cases.
[[nodiscard]] std::optional<math::Matrix4> ReadComponentWorldMatrix(
    std::uintptr_t address) noexcept;

// Reaches the agent through CEntity__GetCAIComponent. Off the game there is no
// such function to call, so it falls back to taking the first component that
// carries an agent pointer -- which is what the fixtures build, and the only
// way any of this is testable without the engine.
[[nodiscard]] std::optional<PawnInfo> ReadPawn(std::uintptr_t address) noexcept;

// Where the skeleton's "Head" and "Spine" bones stand, in world space. Either
// is absent when this skeleton has no such bone. Both come out of one walk
// because finding the component that carries the pose is the expensive half and
// the second bone is then only another pass over an array already in hand.
struct BonePoints {
  std::optional<math::Vec3> head;
  std::optional<math::Vec3> body;
};

// Costs a walk of the entity's components, so it is asked only of what the
// engine drives with a pawn agent rather than of every entity in the world.
[[nodiscard]] BonePoints ReadBonePoints(std::uintptr_t address) noexcept;

}  // namespace afw::games::fc3
