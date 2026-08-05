#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// The update scheduler the census rides on: RunAllGroup is hooked through this
// slot, and what it is handed leads to every entity the frame will touch.
constexpr std::uintptr_t kRunAllGroupSlotAddress{0x115a93c0};

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

}  // namespace afw::games::fc3
