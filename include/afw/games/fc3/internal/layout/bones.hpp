#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// Byte offset, confirmed at 0x11336d60 (ADD EDI,0x100); the decompiler renders
// it as +0x40 because it is pointer arithmetic on the component type. Which
// component the engine picks is decided by a class hash it obtains through a
// virtual call, so the walk here identifies it by the data instead: only the
// right one holds a rigid basis at this offset. Do not replace that with a
// hardcoded vtable without checking the class -- GetPhysComponent is named for
// CCharacterPhysComponent but looks up CGraphicComponent.
constexpr std::uintptr_t kComponentWorldMatrix{0x100};

// The skeleton's pose lives in that same component.
// Entity_GetBoneHandleTableAndBaseMatrix at 0x11336d3a asks for the bone table
// through vtable slot 0x74, but that accessor (0x1017fa51) is two instructions:
// refresh a stale pose, then return component + 0x60. So the table is reachable
// by reading and the call is not needed. Its own array sits at +0x20 of the
// table and its count at +0x24, which is where these land.
constexpr std::uintptr_t kComponentBoneArray{0x80};
constexpr std::uintptr_t kComponentBoneCount{0x84};

// One entry per bone: the name hash at +0x00, then two whole 4x4 matrices --
// the bind pose from +0x10 and the current pose from +0x50. BoneHandleTable_
// GetLocalPos at 0x1059535c reads the translation row of the current one, which
// is 0x30 into it.
constexpr std::uintptr_t kBoneStride{0x90};
constexpr std::uintptr_t kBoneKey{0x00};
constexpr std::uintptr_t kBonePosePosition{0x80};

// Bone names do not survive into the retail build: "ragdoll_head" occurs in the
// whole process exactly once, as the literal Combat_IsHeadshotBoneIndex at
// 0x1047470d compares against, and no loaded skeleton carries a name string at
// all. A bone is therefore only ever named by its hash.
//
// That hash is the engine's own CRC32_CaseSensitive at 0x1008a991 -- table at
// 0x1170b340, the ordinary reflected CRC-32 already reproducing the class-name
// hashes. "Head" hashes to this. Its siblings "Spine" (0x530EC1CB) and "Spine1"
// (0x8F39FA4E) are the two CPlayerCamera__UpdateAimAssist aims at, which is how
// all three were named without a single name in memory to compare against.
//
// "Head" is the joint where the skull meets the neck, so the aim lands at the
// base of the neck rather than in the middle of the head. Measured off a man
// standing, in metres above his own feet: Neck 1.520, Head 1.634, the top of
// him 1.80. Stepping on into the skull was tried and taken back out again; what
// is aimed at is the bone, as it says.
constexpr std::uint32_t kHeadBoneKey{0x07C159A2};

// An animal carries both of these too, which is what makes it aimable at all
// without guessing at its shape. A live one measured 0.93 tall in its own box,
// with the box starting at the ground: its spine stood 0.75 up and its head
// 0.96, while the joints of its legs filled everything from 0.08 to 0.48. The
// middle of that box is therefore 0.465 -- among the legs, a third of the
// animal's height below the body, which is why aiming at a box centre shoots
// under a boar. The spine is where the animal actually is.
constexpr std::uint32_t kSpineBoneKey{0x530EC1CB};

// Tried in turn when the first is missing. A bird is the reason there is a
// chain at all: its box is a flat slab a tenth of a metre tall sitting where
// its own coordinates are, which is not where the bird is, so falling back to
// the middle of that box aims at nothing. Anything that lands on the creature
// beats it, the head included.
constexpr std::uint32_t kSpineOneBoneKey{0x8F39FA4E};

}  // namespace afw::games::fc3
