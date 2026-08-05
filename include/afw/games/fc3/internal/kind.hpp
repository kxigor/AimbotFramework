#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <afw/games/fc3/snapshot.hpp>
#include <cstdint>

namespace afw::games::fc3 {

// What an entity is. Everything here comes from the engine -- the class of the
// entity's agent, the agent type and the faction it was registered with -- and
// nothing from the shape or size of its bounding box. Two measurements say why
// that matters: a boar's box is longer than a car's, and it is also tall and
// narrow enough to pass for a person.
//
// This lives with the game rather than with the overlay because it is a fact
// the engine states, not a decision about drawing. What each kind is coloured,
// and whether it is drawn or aimed at, is the caller's business.
enum class Kind : std::uint8_t {
  Self,
  Enemy,
  Ally,
  Civilian,
  Predator,
  Animal,
  Bird,
  Vehicle,
  Prop
};

// The engine's own predicate, copied out of CPawnAgent::AreArmiesHostile: a
// pawn and an animal both hand the same four-byte block to it, so this decides
// hunting animals as well as hostile humans.
[[nodiscard]] bool ArmiesHostile(const PawnInfo& self,
                                 const PawnInfo& other) noexcept;

// The player, found by the agent type the engine assigns rather than by
// proximity to the camera. Null when the snapshot holds no player, which
// happens between the shot that killed you and the respawn.
[[nodiscard]] const PawnInfo* FindSelf(const EntitySnapshot& snapshot) noexcept;

[[nodiscard]] Kind KindOfPawn(const PawnInfo& pawn,
                              const PawnInfo* self) noexcept;

// An entity without a pawn record is a prop and stays one. Guessing beyond that
// point is what produced red cars and green pirates.
[[nodiscard]] Kind KindOf(const EntityInfo& entity,
                          const PawnInfo* self) noexcept;

[[nodiscard]] const char* KindName(Kind kind) noexcept;

}  // namespace afw::games::fc3
