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

// CEntityUpdaterManager::AddEntityToUpdate at 0x107c5d96 queues an entity and
// then every component of its +0x88 array, so an update group holds both side
// by side and most of what it holds is components. A component does not point
// at its entity directly: +0x04 is a handle shared by all components of the
// same entity, and the entity sits at +0x0C of that handle. Verified live in
// both directions on two unrelated entities -- from 0x53D5D400 out through its
// component array and back, and the same for 0x53D5E060.
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

constexpr std::uintptr_t kPawnIsDead{0x258};
constexpr std::uintptr_t kPawnOwnArmyBit{0x280};
constexpr std::uintptr_t kPawnHostileMask{0x281};
constexpr std::uintptr_t kPawnReverseHostile{0x283};
constexpr std::uintptr_t kPawnArmy{0x284};
constexpr std::uintptr_t kPawnFaction{0x290};
// A hashed name, not a small enum: 0xC940191A was read from a live civilian.
constexpr std::uintptr_t kPawnAgentType{0x294};

}  // namespace afw::games::fc3
