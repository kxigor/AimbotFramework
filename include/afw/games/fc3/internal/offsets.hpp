#pragma once

#include <windows.h>

#include <cstdint>

namespace afw::games::fc3 {

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

constexpr std::uintptr_t kSchedulerGroups{0x20};
constexpr std::uintptr_t kSchedulerGroupCount{0x24};
constexpr std::uintptr_t kGroupEntityGroup{0x08};
constexpr std::uintptr_t kEntityGroupArray{0x04};
constexpr std::uintptr_t kEntityGroupCount{0x08};

constexpr std::uint32_t kMaxGroups{256};
constexpr std::uint32_t kMaxGroupEntities{4096};

// Rows of the row-major world matrix at +0x20; row 3 is the translation.
constexpr std::uintptr_t kEntityAxisX{0x20};
constexpr std::uintptr_t kEntityAxisY{0x30};
constexpr std::uintptr_t kEntityAxisZ{0x40};
constexpr std::uintptr_t kEntityPosition{0x50};
constexpr std::uintptr_t kEntityAabbMin{0x6C};
constexpr std::uintptr_t kEntityAabbMax{0x78};
constexpr std::uintptr_t kPawnIsDead{0x258};
constexpr std::uintptr_t kPawnArmy{0x284};
constexpr std::uintptr_t kPawnFaction{0x290};

// 0 Blue, 1 Red, 2 Grey, 3 Neutral, 4 Green, 5 Yellow.
constexpr std::int32_t kMaxArmy{5};
constexpr std::int32_t kMaxFaction{3};

constexpr std::uintptr_t kPawnOwnArmyBit{0x280};
constexpr std::uintptr_t kPawnHostileMask{0x281};
constexpr std::uintptr_t kPawnReverseHostile{0x283};

// Template and system entities all sit here with an identity transform.
constexpr float kTemplateCoordinate{-5248.0F};
constexpr float kTemplateTolerance{1.0F};
constexpr float kMaxWorldCoordinate{100000.0F};
constexpr float kMinHeight{0.2F};
constexpr float kMaxHeight{20.0F};

// A humanoid's object-space box, from the same family of measurements as the
// height bounds above.
constexpr float kHumanoidMinHeight{1.3F};
constexpr float kHumanoidMaxHeight{2.3F};
constexpr float kHumanoidMaxWidth{1.5F};
constexpr float kHumanoidFootTolerance{0.4F};

constexpr float kUnitTolerance{0.05F};
constexpr float kAxisTolerance{0.05F};
constexpr float kMaxBoxSpan{500.0F};
constexpr float kMinFov{0.1F};
constexpr float kMaxFov{3.0F};

}  // namespace afw::games::fc3
