#pragma once

#include <windows.h>

#include <cstdint>

namespace afw::games::fc3 {

// Where things sit in the game's memory. Everything here is a fact about the
// binary; what counts as a plausible value read from it lives in limits.hpp.

// Far Cry 3 is a 32-bit process, so every pointer it stores is four bytes wide
// whatever this build's uintptr_t happens to be.
using EnginePtr = std::uint32_t;

constexpr PCWSTR kModuleName{L"FC3_d3d11.dll"};
constexpr std::uintptr_t kGhidraImageBase{0x10000000};

constexpr std::uintptr_t kCameraManagerContainer{0x11e137b4};
constexpr std::uintptr_t kViewportBlock{0x11e5a654};

constexpr std::uintptr_t kContainerManagerSlot{0x08};
constexpr std::uintptr_t kContainerLiveFlag{0x0C};
constexpr std::uintptr_t kManagerViewSlot{0x04};
constexpr std::uintptr_t kViewOffset{0x18};

constexpr std::uintptr_t kViewPosition{0x04};
// The frame is drawn from the horizontal angle; the vertical field is
// configured separately and does not follow it.
constexpr std::uintptr_t kViewHorizontalFov{0x24};
constexpr std::uintptr_t kViewVerticalFov{0x2C};
constexpr std::uintptr_t kViewForward{0x64};
constexpr std::uintptr_t kViewUp{0x70};
constexpr std::uintptr_t kViewRight{0x7C};

constexpr std::uintptr_t kViewportWidth{0x10};
constexpr std::uintptr_t kViewportHeight{0x14};

constexpr std::uintptr_t kRunAllGroupSlotAddress{0x115a93c0};

// The engine has no RTTI; class identity lives in a registry. A class
// descriptor and a class definition share a layout -- a count at +0x04 and an
// inline array of ancestor name hashes from +0x08, the class's own hash last.
// CClassDescriptor::MatchesHash at 0x1005a053 is just a walk of that array, so
// only obtaining the descriptor needs the engine: virtual slot 1.
constexpr std::uintptr_t kEntityClassDef{0x11E13600};
constexpr std::uintptr_t kClassDefAncestors{0x04};
constexpr std::uintptr_t kClassDefHashes{0x08};
constexpr std::size_t kClassDescriptorSlot{1};

constexpr std::uintptr_t kSchedulerGroups{0x20};
constexpr std::uintptr_t kSchedulerGroupCount{0x24};
constexpr std::uintptr_t kGroupEntityGroup{0x08};
constexpr std::uintptr_t kEntityGroupArray{0x04};
constexpr std::uintptr_t kEntityGroupCount{0x08};
// Update groups are typed, and the type decides what the array actually holds:
// CEntityUpdateGroup::ForEachEntity builds its mask as 1 << *(group + 0x18)
// before handing each element to the group's own callback. An element is only
// a CEntity for some of those types.
constexpr std::uintptr_t kEntityGroupTypeBit{0x18};

// The row-major world matrix: rows 0..2 are the axes, row 3 at +0x50 is the
// position. Read whole rather than row by row.
//
// This copy is only authoritative for an entity that has no physics component.
// Entity_GetBoneHandleTableAndBaseMatrix at 0x11336d3a picks between the two:
// component + kComponentWorldMatrix when GetPhysComponent finds one, this
// otherwise. Everything that moves has one, which is why reading this alone
// left characters, animals and vehicles without a usable transform.
constexpr std::uintptr_t kEntityTransform{0x20};

constexpr std::uintptr_t kEntityComponentArray{0x88};
constexpr std::uintptr_t kEntityComponentCount{0x8C};

// An update group holds components as well as entities: AddEntityToUpdate at
// 0x107c5d96 queues both. A component reaches its entity through a handle at
// +0x04, then +0x0C of that handle.
constexpr std::uintptr_t kComponentOwnerHandle{0x04};
constexpr std::uintptr_t kHandleEntity{0x0C};

// Byte offset, confirmed at 0x11336d60 (ADD EDI,0x100); the decompiler renders
// it as +0x40 because it is pointer arithmetic on the component type. Which
// component the engine picks is decided by a class hash it obtains through a
// virtual call, so the walk here identifies it by the data instead: only the
// right one holds a rigid basis at this offset. Do not replace that with a
// hardcoded vtable without checking the class -- GetPhysComponent is named for
// CCharacterPhysComponent but looks up CGraphicComponent.
constexpr std::uintptr_t kComponentWorldMatrix{0x100};
// min at +0x6C and max at +0x78, which is one Vec3 apart, so both come in one
// read.
constexpr std::uintptr_t kEntityAabb{0x6C};

// CEntity keeps its own liveness flag well before the pawn fields start.
constexpr std::uintptr_t kEntityIsAlive{0x9A};

// The offsets below are fields only on a CBasePawnEntity, registered by
// GetCBasePawnEntityClassDef at 0x1010e11b. Anywhere else they land inside
// neighbouring pointers, so ask the registry before reading them.
//
// Every one of them is named by CBasePawnEntity_RegisterProperties at
// 0x10bdcac2, which registers "IsDead" at 600, "selArmy" at 0x284,
// "selFaction" at 0x290 and "selPawnAgentType" at 0x294. The mask at +0x280 is
// filled by OnPostLoad_InitArmyMasks, and that registrar attaches it as a
// post-load callback -- so a pawn spawned rather than placed on the map never
// runs it, and ownArmyBit == 1 << army holds for map-placed pawns only. It is
// not a test of whether a pawn is real.
constexpr std::uintptr_t kPawnClassDef{0x11E15440};

// Only IsDead is a field of the entity. selArmy, selFaction and
// selPawnAgentType are registered against a nested "CPawnAgent" group, so they
// are offsets into the AGENT, and reading them off the entity is what left a
// world of 214 entities with two pawn records.
constexpr std::uintptr_t kPawnIsDead{0x258};

// An entity reaches its agent through its CAIComponent, whose +0x14 holds it.
// Finding that component is the engine's job, not ours:
// CEntity__GetCAIComponent takes only `this`, so calling it is safe, whereas
// scanning the component array ourselves means calling virtual slot 1 on
// objects whose class is still unknown -- and that crashed the game inside a
// four-argument function that dereferenced the stack garbage it got instead of
// an argument.
constexpr std::uintptr_t kComponentAgent{0x14};
constexpr std::uintptr_t kGetAiComponent{0x101C7E37};

// GetComponentBySlot searches linearly unless this flag is set, and then it
// builds a hash table and caches it in the entity the first time it is asked --
// an allocation through the game's own CRT, made from our thread, for every
// entity we look at. Observing the world must not change it, so the accessor is
// only called when the answer is already there to be found.
constexpr std::uintptr_t kEntityFlags{0x60};
constexpr std::uintptr_t kEntityHashTable{0x64};
constexpr std::uint32_t kEntityHashTableFlag{0x200000};

// The army block is one four-byte struct -- own bit, hostile mask, alliance
// mask, reverse-hostile mask -- and the engine passes a pawn's and an animal's
// to the same predicate. CPawnAgent keeps it at +0x280, CAnimalAgentFC3 at
// +0x4FE.
constexpr std::uintptr_t kPawnAgentArmyBlock{0x280};
constexpr std::uintptr_t kAnimalAgentArmyBlock{0x4FE};
constexpr std::uintptr_t kArmyBlockHostile{1};
constexpr std::uintptr_t kArmyBlockReverseHostile{3};

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

constexpr std::uintptr_t kAgentArmy{0x284};
constexpr std::uintptr_t kAgentFaction{0x290};
constexpr std::uintptr_t kAgentType{0x294};

constexpr std::uintptr_t kPawnAgentClassDef{0x11E8B36C};
constexpr std::uintptr_t kAnimalAgentClassDef{0x11E8B3D0};
constexpr std::uintptr_t kVehicleAgentClassDef{0x11E8B4BC};

// Birds. Registered against CAgent rather than CGameAgent, so it is not a
// sibling of the three above and carries none of their layout -- the army block
// must not be read off it.
constexpr std::uintptr_t kSimpleAnimalAgentClassDef{0x11E8B49C};
constexpr std::uintptr_t kPawnOwnArmyBit{0x280};
constexpr std::uintptr_t kPawnHostileMask{0x281};
constexpr std::uintptr_t kPawnReverseHostile{0x283};
constexpr std::uintptr_t kPawnArmy{0x284};
constexpr std::uintptr_t kPawnFaction{0x290};
// enumPawnAgentType: Unknown, Player, Companion, Civilian. An earlier note here
// called it a hashed name after reading 0xC940191A live, but that was read off
// an object that was never a pawn, before the class was checked at all.
constexpr std::uintptr_t kPawnAgentType{0x294};

}  // namespace afw::games::fc3
