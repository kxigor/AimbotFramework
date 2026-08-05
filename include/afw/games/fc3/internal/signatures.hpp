#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// Patch recipes rather than layout: each is a byte pattern to find and an
// offset into it to overwrite. They are matched rather than addressed so that a
// patched or relocated image either agrees or reports that it does not.

// Where the shot handler at 0x1041ab6d stores the kick it just worked out.
// FUN_1031c106 is a nine-word struct copy -- __thiscall, destination in ECX,
// source pushed, and it returns with RET 4, so it clears the argument itself.
// Skipping the call therefore has to clear the argument in its place, which is
// what ADD ESP,4 followed by two NOPs does in exactly the five bytes the CALL
// occupied. Nothing else writes the kick, so with the store gone every weapon
// is steady at once, and none of the per-weapon parameter blocks are touched.
//
// The signature covers the argument push, the destination, the two field
// stores and the call, with only the call's own displacement masked out.
constexpr const char* kRecoilStoreSignature{
    "50 8D 4E 78 F3 0F 11 55 E0 F3 0F 11 45 CC C6 45 D0 01 E8 ?? ?? ?? ??"};
constexpr std::uintptr_t kRecoilStorePatchOffset{0x12};

// Every bullet that is not a shotgun pellet leaves along the direction
// CWeaponFireStrategy works out at 0x1051d7bd, and the last thing done to that
// direction is the spread: one random raised to a grouping exponent gives a
// magnitude, a second random over a full turn gives an angle, and the polar
// offset they make is turned into a rotation and multiplied into the aim
// matrix. The magnitude is scaled by the weapon's current spread, which is the
// controller's CurrentBulletSpread at +0x1D4 -- the game names it itself, in
// the property registration at 0x104168a6.
//
// The accessor at 0x10415783 is the only way that field reaches the direction,
// so an FLDZ in place of its FLD hands every caller a zero and the offset
// collapses to nothing. Freezing the field instead would have left it stuck at
// whatever it held when the patch went on, which is not zero after a burst.
//
// The accessor that follows opens with the same three bytes over +0x1D8, so the
// signature has to reach through the displacement to the return to be the only
// one in the image.
//
// Shotguns are not touched: bUseAngleSpread at archetype +0x2A4 sends them down
// the other branch, which spreads by its own pitch and yaw angles instead.
constexpr const char* kSpreadReadSignature{"8B 41 48 D9 80 D4 01 00 00 C3"};
constexpr std::uintptr_t kSpreadReadPatchOffset{0x03};

// Breathing reaches the view only on a weapon whose camera is driven by the
// weapon's own bone -- bIsCameraBoneUsedFromSway at archetype +0x644, which the
// game glosses as "Camera sways together with reticule. Sniper behavior". That
// is why it never showed up in the view angles with anything else in hand: on
// every other weapon it moves the model through the animation rig and never
// touches the angles, and since the shot direction is built from the camera's
// orientation, it does not bend a bullet either.
//
// Where it does reach the view, at 0x1032113f, the camera subtracts the offset
// it applied last frame, refreshes it out of the sway component, and adds the
// new one back. The refresh is an FLD/FSTP pair inside a three-count loop, so
// erasing just those two instructions leaves the loop turning over an unchanged
// offset -- and the same number is then subtracted and added, which is nothing.
// Skipping the whole branch instead would have stopped the subtraction as well
// and frozen the last drift into the view for good.
//
// The signature runs from the loop counter through the loop's own jump.
constexpr const char* kSwayCopySignature{
    "6A 03 8B CB 2B C3 5A D9 04 08 D9 19 83 C1 04 4A 75 F5"};
constexpr std::uintptr_t kSwayCopyPatchOffset{0x07};

// AmmoInClip has one setter, at 0x10417dda, and it is the only thing that
// writes the field -- which is why searching the image for a store through
// +0xCC finds nothing: the setter takes the address into EAX first and writes
// through that. A hardware watchpoint on the field found it in one burst.
//
// It already compares the incoming value with the standing one, to decide
// whether anything needs marking as changed. Turning that comparison's jump
// from "equal, skip the marking" into "less, skip the write" costs two bytes
// and leaves everything else alone: a round spent is ignored, while a reload
// or a fresh weapon, which raise the count, still go through. Erasing the
// write instead would have frozen the magazine wherever it stood, including
// at zero.
constexpr const char* kAmmoStoreSignature{
    "55 8B EC 8B 55 08 8D 81 CC 00 00 00 3B 10 74 07 8B 49 60 C6 41 44 01 89 "
    "10"};
constexpr std::uintptr_t kAmmoStorePatchOffset{0x0E};

// The shot handler asks its fire strategy whether this shot needs a round, and
// only then looks at AmmoInClip. Erasing the look leaves the flags standing
// from the TEST AL,AL that got here, and on this path AL is what carried the
// branch, so it cannot be zero -- the JNZ after it therefore always takes the
// firing road and the magazine is never found empty.
//
// Nothing else is touched: rounds still leave the magazine, reloads still
// happen, the count on screen still falls. What goes is the refusal to fire at
// the bottom of it.
constexpr const char* kAmmoCheckSignature{
    "8B 8E E8 00 00 00 8B 01 FF 50 68 84 C0 74 24 83 BE CC 00 00 00 00 75 1B"};
constexpr std::uintptr_t kAmmoCheckPatchOffset{0x0F};

}  // namespace afw::games::fc3
