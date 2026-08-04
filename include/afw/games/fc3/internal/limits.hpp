#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// What counts as a plausible value once it has been read. Nothing here says
// where anything lives -- that is layout.hpp. These are the numbers Classify
// judges by, and a wrong offset shows up as a value failing one of them rather
// than as a walk of the address space.

// A bad count would otherwise turn one misread field into an unbounded walk.
constexpr std::uint32_t kMaxGroups{256};
constexpr std::uint32_t kMaxGroupEntities{4096};
constexpr std::uint32_t kMaxComponents{128};
constexpr std::uint32_t kMaxAncestors{32};

// Calling through a vtable is only safe if the vtable is the game's own, so the
// pointer has to land inside its image. The module is 37 MB; this bounds it
// without needing the exact size.
constexpr std::uintptr_t kMaxModuleSpan{0x4000000};

// One entry per distinct class seen, so the engine is asked once per class
// rather than once per object.
constexpr std::size_t kClassCacheSlots{256};

// What a class-descriptor getter is allowed to look like before we call it.
// Every real one is a handful of instructions ending in a bare RET; a window
// this wide covers them and still stops well short of anything substantial.
constexpr std::uint32_t kGetterScanBytes{128};
constexpr std::uint32_t kMaxThunkHops{4};
constexpr std::uint8_t kRelativeJump{0xE9};
constexpr std::uint8_t kReturn{0xC3};
constexpr std::uint8_t kReturnWithOperand{0xC2};
constexpr std::uintptr_t kRelativeJumpSize{5};

// The names come from the enum CBasePawnEntity_RegisterProperties at 0x10bdcac2
// registers alongside the field: BlueGuys, RedGuys, GreyGuys, NeutralGuys,
// GreenGuys, YellowGuys.
constexpr std::int32_t kMaxArmy{5};

constexpr std::uint32_t kMaxAgentType{3};
// 0 Unknown, 1 Rebel, 2 Pirate, 3 Privateer.
constexpr std::int32_t kMaxFaction{3};
// A live civilian read -2 here, so the range starts below zero.
constexpr std::int32_t kNoFaction{-2};

// Loose on purpose: the engine stores already normalised vectors, and garbage
// misses by orders rather than by hundredths.
constexpr float kUnitTolerance{0.05F};
constexpr float kAxisTolerance{0.05F};
constexpr float kMaxBoxSpan{500.0F};

// Template and system entities all sit here with an identity transform.
constexpr float kTemplateCoordinate{-5248.0F};
constexpr float kTemplateTolerance{1.0F};
constexpr float kMaxWorldCoordinate{100000.0F};

// A humanoid's object-space box, measured live: min (-0.6,-0.6,0),
// max (+0.6,+0.6,+1.70).
constexpr float kHumanoidMinHeight{1.3F};
constexpr float kHumanoidMaxHeight{2.3F};
constexpr float kHumanoidMaxWidth{1.5F};
constexpr float kHumanoidFootTolerance{0.4F};

constexpr float kMinFov{0.1F};
constexpr float kMaxFov{3.0F};

}  // namespace afw::games::fc3
