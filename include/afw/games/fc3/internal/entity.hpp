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

}  // namespace afw::games::fc3
