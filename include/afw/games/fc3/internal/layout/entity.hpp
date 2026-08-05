#pragma once

#include <cstddef>
#include <cstdint>

namespace afw::games::fc3 {

// The engine has no RTTI; class identity lives in a registry. A class
// descriptor and a class definition share a layout -- a count at +0x04 and an
// inline array of ancestor name hashes from +0x08, the class's own hash last.
// CClassDescriptor::MatchesHash at 0x1005a053 is just a walk of that array, so
// only obtaining the descriptor needs the engine: virtual slot 1.
constexpr std::uintptr_t kEntityClassDef{0x11E13600};
constexpr std::uintptr_t kClassDefAncestors{0x04};
constexpr std::uintptr_t kClassDefHashes{0x08};
constexpr std::size_t kClassDescriptorSlot{1};

// The row-major world matrix: rows 0..2 are the axes, row 3 at +0x50 is the
// position. Read whole rather than row by row.
//
// This copy is only authoritative for an entity that has no physics component.
// Entity_GetBoneHandleTableAndBaseMatrix at 0x11336d3a picks between the two:
// component + kComponentWorldMatrix when GetPhysComponent finds one, this
// otherwise. Everything that moves has one, which is why reading this alone
// left characters, animals and vehicles without a usable transform.
constexpr std::uintptr_t kEntityTransform{0x20};

// min at +0x6C and max at +0x78, which is one Vec3 apart, so both come in one
// read.
constexpr std::uintptr_t kEntityAabb{0x6C};

constexpr std::uintptr_t kEntityComponentArray{0x88};
constexpr std::uintptr_t kEntityComponentCount{0x8C};

// An update group holds components as well as entities: AddEntityToUpdate at
// 0x107c5d96 queues both. A component reaches its entity through a handle at
// +0x04, then +0x0C of that handle.
constexpr std::uintptr_t kComponentOwnerHandle{0x04};
constexpr std::uintptr_t kHandleEntity{0x0C};

// GetComponentBySlot searches linearly unless this flag is set, and then it
// builds a hash table and caches it in the entity the first time it is asked --
// an allocation through the game's own CRT, made from our thread, for every
// entity we look at. Observing the world must not change it, so the accessor is
// only called when the answer is already there to be found.
constexpr std::uintptr_t kEntityFlags{0x60};
constexpr std::uintptr_t kEntityHashTable{0x64};
constexpr std::uint32_t kEntityHashTableFlag{0x200000};

}  // namespace afw::games::fc3
