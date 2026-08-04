#pragma once

#include <atomic>

namespace afw::config {

// What the overlay is allowed to draw, one flag per kind. The dead are not on
// this list on purpose: a corpse keeps the flag of whatever it was, so hiding
// it would take an entry away from the kind it belongs to.
constexpr float kDefaultAimSmoothing{0.35F};
// The whole cone, not half of it, which is why the default is twice what the
// half-angle used to be. At 360 nothing is outside it and the assist considers
// the entire world, including whoever is behind you.
constexpr float kDefaultAimCone{50.0F};
constexpr float kDefaultAimRange{150.0F};
constexpr float kDefaultAutoFireCone{2.0F};

constexpr float kMinAimSmoothing{0.02F};
constexpr float kMaxAimSmoothing{1.0F};
constexpr float kMinAimCone{1.0F};
constexpr float kMaxAimCone{360.0F};
constexpr float kMinAimRange{10.0F};
constexpr float kMaxAimRange{300.0F};
constexpr float kMinAutoFireCone{0.1F};
constexpr float kMaxAutoFireCone{15.0F};
constexpr float kMaxWeight{1.0F};
constexpr float kMaxPriority{1.0F};
// Where each kind starts out once the priority term is given any weight at all:
// a man above a beast, and a bird under both, because it is the one target the
// assist has no business leaving a fight for.
constexpr float kDefaultAnimalPriority{0.5F};
constexpr float kDefaultBirdPriority{0.25F};
constexpr float kDefaultVehiclePriority{0.5F};

struct Settings {
  std::atomic<bool> espEnabled{true};

  std::atomic<bool> espEnemies{true};
  std::atomic<bool> espAllies{true};
  std::atomic<bool> espCivilians{true};
  std::atomic<bool> espAnimals{true};
  std::atomic<bool> espBirds{true};
  std::atomic<bool> espVehicles{true};
  // The cube drawn where the aim will go: the head on a man, the spine on an
  // animal or a bird. One switch, because it is one thing being shown.
  std::atomic<bool> espAimBoxes{true};
  // Everything the engine would not name. Off by default: it is most of the
  // world, and near the player it is mostly your own equipment.
  std::atomic<bool> espProps{false};

  // Held rather than toggled, so the view is the player's again the moment the
  // button comes up.
  std::atomic<bool> aimEnabled{true};
  std::atomic<bool> aimEnemies{true};
  std::atomic<bool> aimAnimals{false};
  std::atomic<bool> aimBirds{false};
  std::atomic<bool> aimVehicles{false};
  std::atomic<bool> noRecoil{false};
  std::atomic<bool> noSway{false};
  std::atomic<bool> noSpread{false};
  // Pulls the trigger once the aim is inside autoFireCone of the target. Held
  // down while it stays there, released the moment it does not, and never
  // pressed at all unless this window is the one with focus -- otherwise the
  // click lands in whatever you alt-tabbed to.
  std::atomic<bool> autoFire{false};
  std::atomic<float> autoFireCone{kDefaultAutoFireCone};

  // The fraction of the remaining correction applied each frame. At 1 the view
  // snaps, which no hand does.
  std::atomic<float> aimSmoothing{kDefaultAimSmoothing};
  std::atomic<float> aimConeDegrees{kDefaultAimCone};
  std::atomic<float> aimRange{kDefaultAimRange};

  // How the candidates inside the cone are ranked against each other. Each
  // term is worked out as a 0..1 score before its weight applies, so the
  // weights are comparable and any of them may be turned off on its own. With
  // only aimWeightAim set this picks whoever the eye already points nearest to,
  // which is what it did before there was a formula.
  std::atomic<float> aimWeightAim{kMaxWeight};
  std::atomic<float> aimWeightDistance{0.0F};
  std::atomic<float> aimWeightPriority{0.0F};

  // What each kind is worth to the priority term. Only consulted when that
  // term carries weight.
  std::atomic<float> aimPriorityEnemies{kMaxPriority};
  std::atomic<float> aimPriorityAnimals{kDefaultAnimalPriority};
  std::atomic<float> aimPriorityBirds{kDefaultBirdPriority};
  std::atomic<float> aimPriorityVehicles{kDefaultVehiclePriority};

  bool menuVisible{true};
  bool unloadRequested{false};
};

[[nodiscard]] Settings& Current() noexcept;

}  // namespace afw::config
