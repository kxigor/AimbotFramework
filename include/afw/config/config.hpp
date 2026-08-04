#pragma once

#include <atomic>

namespace afw::config {

constexpr float kDefaultAimSmoothing{1.0F};

constexpr float kDefaultWeightAim{0.60F};
constexpr float kDefaultWeightPriority{0.30F};
constexpr float kDefaultWeightDistance{0.05F};
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
  std::atomic<bool> espAimBoxes{true};
  std::atomic<bool> espProps{false};
  std::atomic<bool> espPanel{false};

  std::atomic<bool> aimEnabled{true};
  std::atomic<bool> aimEnemies{true};
  std::atomic<bool> aimAnimals{false};
  std::atomic<bool> aimBirds{false};
  std::atomic<bool> aimVehicles{false};
  std::atomic<bool> noRecoil{false};
  std::atomic<bool> noSway{false};
  std::atomic<bool> noSpread{false};
  std::atomic<bool> infiniteAmmo{false};
  std::atomic<bool> traceProbe{false};

  std::atomic<bool> autoFire{false};
  std::atomic<float> autoFireCone{kDefaultAutoFireCone};

  std::atomic<float> aimSmoothing{kDefaultAimSmoothing};
  std::atomic<float> aimConeDegrees{kDefaultAimCone};
  std::atomic<float> aimRange{kDefaultAimRange};

  std::atomic<float> aimWeightAim{kDefaultWeightAim};
  std::atomic<float> aimWeightDistance{kDefaultWeightDistance};
  std::atomic<float> aimWeightPriority{kDefaultWeightPriority};
  std::atomic<float> aimWeightReach{kMaxWeight};

  std::atomic<float> aimPriorityEnemies{kMaxPriority};
  std::atomic<float> aimPriorityAnimals{kDefaultAnimalPriority};
  std::atomic<float> aimPriorityBirds{kDefaultBirdPriority};
  std::atomic<float> aimPriorityVehicles{kDefaultVehiclePriority};

  bool menuVisible{true};
  bool unloadRequested{false};
};

[[nodiscard]] Settings& Current() noexcept;

}  // namespace afw::config
