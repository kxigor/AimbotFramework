#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// The collision world singleton and the query that walks it. The query is
// __thiscall on the world -- it reaches its own core through +0x90 -- takes
// twelve stack arguments and returns with RET 0x30, so it clears them itself.
// Its first stack argument is pushed by every caller and read by nobody: the
// forwarder passes on the eleven after it.
//
// The order was taken off the running game with a breakpoint on both the
// forwarder and what it forwards to, not from the decompiler, whose prototype
// has the out buffer sitting in the slot it labels flags:
//
//   (ignored, flags = 1, out, from, delta, filter = 0, 4, 0, 0, 1, 0, 0)
//
// `delta` is the segment's vector and not its far end -- the walk runs from
// `from` to `from + delta`.
constexpr std::uintptr_t kCollisionQueryWorld{0x11E660BC};
constexpr std::uintptr_t kSegmentQuery{0x10761940};

// What the walk found is filed against the id the query handed back, and
// getting it out again is all reads: FUN_1077583a at 0x1077583a walks two
// tables of 0x624-byte records hanging off the query core, matching the id it
// keeps in the record's first word, and hands back the pair two words in --
// the base of the hits and how many there are.
//
// One hit is 0x30 bytes. The shot handler at 0x104636f3 reads the point out of
// the front of it, resolves the entity through the handle at +0x18, and asks
// Combat_IsHeadshotBoneIndex about the bone at +0x22; +0x23 is the surface it
// struck.
constexpr std::uintptr_t kQueryCore{0x90};
constexpr std::uintptr_t kQueryTableA{0x9C};
constexpr std::uintptr_t kQueryCountA{0xA0};
constexpr std::uintptr_t kQueryTableB{0xA8};
constexpr std::uintptr_t kQueryCountB{0xAC};
constexpr std::uintptr_t kQueryRecordStride{0x624};
constexpr std::uintptr_t kQueryRecordHits{0x08};

constexpr std::uintptr_t kHitStride{0x30};

// The filter the query is given is an object, and the walk folds two of its
// words into the mask it tests with: (word at +8) << 0x12 | (word at +4) |
// 0x10000. Handing it zeros asks for a mask no layer answers, and every walk
// comes back clear -- with a car three metres away.
//
// These are what the game's own segment queries carry, read off a live frame
// whose start point matched the player's logged position to the centimetre.
constexpr std::uintptr_t kQueryFilterVtable{0x11568158};
constexpr std::uint32_t kQueryFilterMask{0x11};

}  // namespace afw::games::fc3
