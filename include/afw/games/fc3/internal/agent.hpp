#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

// Reaches the agent through CEntity__GetCAIComponent, establishes its class
// through the registry, and hands the fields to ReadAgent. Off the game there
// is no accessor to call, so it falls back to taking the first component that
// carries an agent pointer -- which is what the fixtures build, and the only
// way any of this is testable without the engine.
[[nodiscard]] std::optional<PawnInfo> ReadPawn(std::uintptr_t address) noexcept;

}  // namespace afw::games::fc3
