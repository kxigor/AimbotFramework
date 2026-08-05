#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// The basis the frame is actually drawn from, hanging off the camera manager.
constexpr std::uintptr_t kViewPosition{0x04};
// The frame is drawn from the horizontal angle; the vertical field is
// configured separately and does not follow it.
constexpr std::uintptr_t kViewHorizontalFov{0x24};
constexpr std::uintptr_t kViewVerticalFov{0x2C};
constexpr std::uintptr_t kViewForward{0x64};
constexpr std::uintptr_t kViewUp{0x70};
constexpr std::uintptr_t kViewRight{0x7C};

// The camera the player looks through is one of the player entity's own
// components: CPlayerCamera::GetCameraPosition at 0x104301a0 reaches the player
// back through +0x04 -> +0x0C, the same owner handle every component carries.
// Its state object hangs off +0x24, and three copies of the view angles live
// inside that. The look update at 0x1047e89c reads kCameraAnglesSource
// (GetCameraStateB's block at 0x4E0, plus 0xA4), adds the frame's mouse
// movement, and commits the result to kCameraAnglesCommit (state A at 0x2D0,
// plus 0xB0). Writing all three is what makes a new heading survive the frame.
constexpr std::uintptr_t kCameraState{0x24};
constexpr std::uintptr_t kCameraAnglesA{0x374};
constexpr std::uintptr_t kCameraAnglesCommit{0x380};
constexpr std::uintptr_t kCameraAnglesSource{0x584};

// Pitch first, then roll, then yaw. The disassembly of the look update reads
// the other way round -- the live camera settled it, because the axis it takes
// from the aim controller's +0x28 turned out to be the vertical one, not the
// horizontal. Yaw is measured a quarter turn off atan2 over the world's x and
// y, so converting a direction into one is not just an arctangent.
constexpr std::uintptr_t kViewAnglePitch{0x00};
constexpr std::uintptr_t kViewAngleYaw{0x08};

// Firing pushes the view up through a decaying term the engine keeps beside the
// angles, once per block: state A's +0xF4 and the source block's +0xF4. Named
// by suppression rather than by the binary, which calls it nothing: held at
// zero, six shots produced not one frame in which the pitch rose, against a
// 0.29 degree kick over two shots with them left alone. The three candidates
// the disassembly suggested first -- the decaying triple at camera +0x120 --
// never left zero across ten seconds of play, so they are some other effect.
constexpr std::uintptr_t kCameraRecoilA{0x3C4};
constexpr std::uintptr_t kCameraRecoilSource{0x5D4};

}  // namespace afw::games::fc3
