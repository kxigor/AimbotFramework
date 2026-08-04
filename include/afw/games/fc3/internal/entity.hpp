#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

// Exactly what an entity's memory holds, before anything has been decided about
// it. Reading and judging are separate because the read is what fails when the
// address is not memory at all, while the judgement is a pure question about
// bytes already in hand.
struct RawEntity {
  std::uintptr_t address{};
  std::uintptr_t vtable{};
  Matrix4 transform{};
  Aabb box{};
};

// The engine reaches a moving entity's world matrix through a component rather
// than through the entity itself, and picks the component by class hash -- a
// virtual call we cannot make from a read-only walk. This asks the same
// question of the data instead: of the entity's components, the one whose
// matrix slot holds an actually rigid basis is the one the engine would have
// returned. Self-validating, and it needs no class constant to be right.
[[nodiscard]] std::optional<Matrix4> ReadComponentWorldMatrix(
    std::uintptr_t address) noexcept;

// The update groups hold objects of several classes side by side, and reading
// one class's offsets out of another yields nonsense that only looks like a
// failed entity. This asks the engine's own type registry instead, through the
// one virtual it takes to obtain a class descriptor. Answers are cached per
// vtable, so the call happens once per class, not once per object.
//
// Returns false when the game module is absent, which is every unit test: the
// call is never made outside the game.
[[nodiscard]] bool IsEntityClass(std::uintptr_t address) noexcept;

// The same question asked of CBasePawnEntity instead, and the gate the pawn
// offsets need: they are fields on that class and rubbish anywhere else.
[[nodiscard]] bool IsPawnClass(std::uintptr_t address) noexcept;

// ReadPawn reaches the agent through CEntity__GetCAIComponent. Off the game
// there is no such function to call, so it falls back to taking the first
// component that carries an agent pointer -- which is what the fixtures build,
// and the only way any of this is testable without the engine.

// A fallback, not the first choice. Several components of one entity carry a
// rigid basis at kComponentWorldMatrix and they do not agree: measured across
// 33 characters, taking the first rigid one put a single pirate 118 m from
// where he stood, which is why his box never appeared on him. The entity's own
// matrix agreed with the truth in all 33 cases, so this only runs when that one
// is not rigid.

// Walks a component back to its entity. What comes back is confirmed to be an
// entity before it is handed out.
[[nodiscard]] std::optional<std::uintptr_t> OwnerOfComponent(
    std::uintptr_t address) noexcept;

[[nodiscard]] std::expected<RawEntity, std::error_code> ReadEntity(
    std::uintptr_t address) noexcept;

// Touches no memory: everything it decides on is in raw.
[[nodiscard]] std::expected<EntityInfo, Rejection> Classify(
    const RawEntity& raw) noexcept;

// The army fields sit past the end of a plain CEntity, so this is only worth
// calling once the shape says the object is large enough to have them.
[[nodiscard]] std::optional<PawnInfo> ReadPawn(std::uintptr_t address) noexcept;

}  // namespace afw::games::fc3
