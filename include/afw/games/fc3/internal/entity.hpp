#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>

namespace afw::games::fc3 {

// Why an entity was not stored, so the walk can count the rejects apart instead
// of reporting one number that means both "empty list" and "everything failed".
enum class Verdict : std::uint8_t { NotEntity, NotInTheWorld, Stored };

[[nodiscard]] Verdict InspectEntity(std::uintptr_t entity,
                                    EntityInfo& info) noexcept;

}  // namespace afw::games::fc3
