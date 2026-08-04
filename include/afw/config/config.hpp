#pragma once

#include <atomic>

namespace afw::config {

// What the overlay is allowed to draw, one flag per kind. The dead are not on
// this list on purpose: a corpse keeps the flag of whatever it was, so hiding
// it would take an entry away from the kind it belongs to.
constexpr float kDefaultAimSmoothing{0.35F};
constexpr float kDefaultAimCone{25.0F};
constexpr float kDefaultAimRange{150.0F};

constexpr float kMinAimSmoothing{0.02F};
constexpr float kMaxAimSmoothing{1.0F};
constexpr float kMinAimCone{1.0F};
constexpr float kMaxAimCone{90.0F};
constexpr float kMinAimRange{10.0F};
constexpr float kMaxAimRange{300.0F};

struct Settings {
  std::atomic<bool> espEnabled{true};

  std::atomic<bool> espEnemies{true};
  std::atomic<bool> espAllies{true};
  std::atomic<bool> espCivilians{true};
  std::atomic<bool> espAnimals{true};
  std::atomic<bool> espBirds{true};
  std::atomic<bool> espVehicles{true};
  std::atomic<bool> espHeadBoxes{true};
  // Everything the engine would not name. Off by default: it is most of the
  // world, and near the player it is mostly your own equipment.
  std::atomic<bool> espProps{false};

  // Held rather than toggled, so the view is the player's again the moment the
  // button comes up.
  std::atomic<bool> aimEnabled{true};
  std::atomic<bool> aimEnemies{true};
  std::atomic<bool> aimAnimals{false};
  // The fraction of the remaining correction applied each frame. At 1 the view
  // snaps, which no hand does.
  std::atomic<float> aimSmoothing{kDefaultAimSmoothing};
  std::atomic<float> aimConeDegrees{kDefaultAimCone};
  std::atomic<float> aimRange{kDefaultAimRange};

  bool menuVisible{true};
  bool unloadRequested{false};
};

[[nodiscard]] Settings& Current() noexcept;

}  // namespace afw::config
