#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// An entity reaches its agent through its CAIComponent, whose +0x14 holds it.
// Finding that component is the engine's job, not ours:
// CEntity__GetCAIComponent takes only `this`, so calling it is safe, whereas
// scanning the component array ourselves means calling virtual slot 1 on
// objects whose class is still unknown -- and that crashed the game inside a
// four-argument function that dereferenced the stack garbage it got instead of
// an argument.
constexpr std::uintptr_t kComponentAgent{0x14};
constexpr std::uintptr_t kGetAiComponent{0x101C7E37};

// The fields below are the agent's, not the entity's. CBasePawnEntity_
// RegisterProperties at 0x10bdcac2 registers "selArmy" at 0x284, "selFaction"
// at 0x290 and "selPawnAgentType" at 0x294 -- but against a nested "CPawnAgent"
// group, so they are offsets into the AGENT. Reading them off the entity is
// what left a world of 214 entities with two pawn records.
//
// The mask at +0x280 is filled by OnPostLoad_InitArmyMasks, which that same
// registrar attaches as a post-load callback -- so a pawn spawned rather than
// placed on the map never runs it, and ownArmyBit == 1 << army holds for
// map-placed pawns only. It is not a test of whether a pawn is real.

// The army block is one four-byte struct -- own bit, hostile mask, alliance
// mask, reverse-hostile mask -- and the engine passes a pawn's and an animal's
// to the same predicate. CPawnAgent keeps it at +0x280, CAnimalAgentFC3 at
// +0x4FE.
constexpr std::uintptr_t kPawnAgentArmyBlock{0x280};
constexpr std::uintptr_t kAnimalAgentArmyBlock{0x4FE};
constexpr std::uintptr_t kArmyBlockHostile{1};
constexpr std::uintptr_t kArmyBlockReverseHostile{3};

constexpr std::uintptr_t kAgentArmy{0x284};
constexpr std::uintptr_t kAgentFaction{0x290};
// enumPawnAgentType: Unknown, Player, Companion, Civilian. An earlier note here
// called it a hashed name after reading 0xC940191A live, but that was read off
// an object that was never a pawn, before the class was checked at all.
constexpr std::uintptr_t kAgentType{0x294};

// CPawnAgent::IsDead, vtable slot 0xC0 at 0x10bd5c65, is a pure data read once
// the agent is not a Companion: the byte at +0x258 or an army-member state
// of 7. A Companion delegates to the buddy subsystem, so death is not
// established for one. CAnimalAgentFC3::IsDead at 0x10b7789d is a single byte
// at +0x1d0c.
constexpr std::uintptr_t kAgentIsDead{0x258};
constexpr std::uintptr_t kAgentArmyMemberState{0x2B0};
constexpr std::int32_t kArmyMemberStateDead{7};
constexpr std::uintptr_t kAnimalAgentIsDead{0x1D0C};
// CAnimalAgentSimple::IsDead at 0x10b7c99b is two instructions long: read the
// byte, return. Its own offset, because the class descends from CAgent and
// shares no layout with the other three.
constexpr std::uintptr_t kSimpleAnimalAgentIsDead{0x178};

constexpr std::uintptr_t kPawnAgentClassDef{0x11E8B36C};
constexpr std::uintptr_t kAnimalAgentClassDef{0x11E8B3D0};
constexpr std::uintptr_t kVehicleAgentClassDef{0x11E8B4BC};

// Birds. Registered against CAgent rather than CGameAgent, so it is not a
// sibling of the three above and carries none of their layout -- the army block
// must not be read off it.
constexpr std::uintptr_t kSimpleAnimalAgentClassDef{0x11E8B49C};

}  // namespace afw::games::fc3
