#pragma once

#include <afw/games/fc3/internal/structs.hpp>
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

enum class Rejection : std::uint8_t { NotAnEntity, OutsideTheWorld };

[[nodiscard]] std::expected<RawEntity, std::error_code> ReadEntity(
    std::uintptr_t address) noexcept;

// Touches no memory: everything it decides on is in raw.
[[nodiscard]] std::expected<EntityInfo, Rejection> Classify(
    const RawEntity& raw) noexcept;

// The army fields sit past the end of a plain CEntity, so this is only worth
// calling once the shape says the object is large enough to have them.
[[nodiscard]] std::optional<PawnInfo> ReadPawn(std::uintptr_t address) noexcept;

}  // namespace afw::games::fc3
