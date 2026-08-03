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

[[nodiscard]] std::expected<RawEntity, std::error_code> ReadEntity(
    std::uintptr_t address) noexcept;

// Touches no memory: everything it decides on is in raw.
[[nodiscard]] std::expected<EntityInfo, Rejection> Classify(
    const RawEntity& raw) noexcept;

// The army fields sit past the end of a plain CEntity, so this is only worth
// calling once the shape says the object is large enough to have them.
[[nodiscard]] std::optional<PawnInfo> ReadPawn(std::uintptr_t address) noexcept;

}  // namespace afw::games::fc3
