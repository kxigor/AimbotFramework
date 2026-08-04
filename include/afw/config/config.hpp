#pragma once

#include <atomic>

namespace afw::config {

// What the overlay is allowed to draw, one flag per kind. The dead are not on
// this list on purpose: a corpse keeps the flag of whatever it was, so hiding
// it would take an entry away from the kind it belongs to.
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

  bool menuVisible{true};
  bool unloadRequested{false};
};

[[nodiscard]] Settings& Current() noexcept;

}  // namespace afw::config
