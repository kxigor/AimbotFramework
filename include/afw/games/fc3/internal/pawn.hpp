#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

// Reads the fields of an agent whose class has already been established. Which
// offsets are valid depends entirely on that class -- CAnimalAgentFC3 keeps its
// army block 0x27E bytes further along than CPawnAgent, and CAnimalAgentSimple
// descends from CAgent and shares no layout with either -- so the kind is an
// argument here and never a guess.
//
// A field that reads back nonsense costs that field and not the record: what
// the record exists for is the agent's class, and that was established before
// any of these was read. Dropping the whole record instead is how an entity
// ended up classified by the size of its bounding box.
[[nodiscard]] std::optional<PawnInfo> ReadAgent(std::uintptr_t agent,
                                                AgentKind kind) noexcept;

}  // namespace afw::games::fc3
