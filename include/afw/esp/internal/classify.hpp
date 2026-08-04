#pragma once

#include <afw/games/fc3/fc3.hpp>
#include <cstdint>

namespace afw::esp {

// What an entity is, as far as the overlay is concerned. Everything here comes
// from the engine -- the class of the entity's agent, the agent type and the
// faction it was registered with -- and nothing from the shape or size of its
// bounding box. Two measurements say why that matters: a boar's box is longer
// than a car's, and it is also tall and narrow enough to pass for a person.
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
[[nodiscard]] bool ArmiesHostile(const games::fc3::PawnInfo& self,
                                 const games::fc3::PawnInfo& other) noexcept;

// The player, found by the agent type the engine assigns rather than by
// proximity to the camera. Null when the snapshot holds no player, which
// happens between the shot that killed you and the respawn.
[[nodiscard]] const games::fc3::PawnInfo* FindSelf(
    const games::fc3::EntitySnapshot& snapshot) noexcept;

[[nodiscard]] Kind KindOfPawn(const games::fc3::PawnInfo& pawn,
                              const games::fc3::PawnInfo* self) noexcept;

// An entity without a pawn record is a prop and stays one. Guessing beyond that
// point is what produced red cars and green pirates.
[[nodiscard]] Kind KindOf(const games::fc3::EntityInfo& entity,
                          const games::fc3::PawnInfo* self) noexcept;

[[nodiscard]] std::uint32_t ColourOf(const games::fc3::EntityInfo& entity,
                                     Kind kind) noexcept;

[[nodiscard]] const char* KindName(Kind kind) noexcept;

}  // namespace afw::esp
