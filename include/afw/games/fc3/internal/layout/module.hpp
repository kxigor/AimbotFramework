#pragma once

#include <windows.h>

#include <cstdint>

namespace afw::games::fc3 {

// Where things sit in the game's memory. Everything under layout/ is a fact
// about the binary; what counts as a plausible value read from it lives in
// limits.hpp.

// Far Cry 3 is a 32-bit process, so every pointer it stores is four bytes wide
// whatever this build's uintptr_t happens to be.
using EnginePtr = std::uint32_t;

constexpr PCWSTR kModuleName{L"FC3_d3d11.dll"};
constexpr std::uintptr_t kGhidraImageBase{0x10000000};

// The chain the camera is reached through, and the block the game records its
// own render size in. Both are module globals, so both are Ghidra addresses
// that Resolve turns into live ones.
constexpr std::uintptr_t kCameraManagerContainer{0x11e137b4};
constexpr std::uintptr_t kViewportBlock{0x11e5a654};

constexpr std::uintptr_t kContainerManagerSlot{0x08};
constexpr std::uintptr_t kContainerLiveFlag{0x0C};
constexpr std::uintptr_t kManagerViewSlot{0x04};
constexpr std::uintptr_t kViewOffset{0x18};

constexpr std::uintptr_t kViewportWidth{0x10};
constexpr std::uintptr_t kViewportHeight{0x14};

}  // namespace afw::games::fc3
