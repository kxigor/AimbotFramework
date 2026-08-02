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

// 0 Blue, 1 Red, 2 Grey, 3 Neutral, 4 Green, 5 Yellow.
constexpr std::int32_t kMaxArmy{5};
// 0 Unknown, 1 Rebel, 2 Pirate, 3 Privateer.
constexpr std::int32_t kMaxFaction{3};

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
