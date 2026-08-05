#pragma once

#include <afw/games/fc3/internal/layout.hpp>
#include <afw/process/reader.hpp>

namespace afw::games::fc3 {

// Every read of the game's memory goes through these two. Reading a field that
// is not there is the ordinary answer rather than an error worth a code: the
// walk is over objects whose class is not yet established, so a miss means
// "not this one", and the only caller that ever wanted the error code was
// counting reads it had already given up on.
//
// Engine carries the pointer width with it. Far Cry 3 is a 32-bit process, so
// a pointer it stores is four bytes wide whatever this build's uintptr_t is,
// and getting that wrong reads half of one pointer and half of the next.
using Engine = process::Reader<EnginePtr>;

// A base pointer and a count that were validated together, so the bound is
// checked once where it is read instead of at every loop that follows. A count
// misread out of a foreign object is what turns one wrong offset into a walk
// of the address space.
using EngineArray = process::PointerArray<EnginePtr>;

}  // namespace afw::games::fc3
