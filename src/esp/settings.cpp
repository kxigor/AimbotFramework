#include <afw/config/config.hpp>
#include <afw/esp/internal/classify.hpp>
#include <afw/esp/internal/settings.hpp>
#include <optional>

namespace afw::esp {

namespace {

[[nodiscard]] std::optional<float> Ranked(bool wanted,
                                          float priority) noexcept {
  return wanted ? std::optional<float>{priority} : std::nullopt;
}

}  // namespace

bool Shown(Kind kind) noexcept {
  const config::Settings& kSettings{config::Current()};
  switch (kind) {
    case Kind::Enemy:
      return kSettings.espEnemies.load();
    case Kind::Ally:
      return kSettings.espAllies.load();
    case Kind::Civilian:
      return kSettings.espCivilians.load();
    case Kind::Predator:
    case Kind::Animal:
      return kSettings.espAnimals.load();
    case Kind::Bird:
      return kSettings.espBirds.load();
    case Kind::Vehicle:
      return kSettings.espVehicles.load();
    case Kind::Prop:
      return kSettings.espProps.load();
    case Kind::Self:
    default:
      return false;
  }
}

std::optional<float> AimPriority(Kind kind) noexcept {
  const config::Settings& kSettings{config::Current()};
  switch (kind) {
    case Kind::Enemy:
      return Ranked(kSettings.aimEnemies.load(),
                    kSettings.aimPriorityEnemies.load());
    case Kind::Predator:
    case Kind::Animal:
      return Ranked(kSettings.aimAnimals.load(),
                    kSettings.aimPriorityAnimals.load());
    case Kind::Bird:
      return Ranked(kSettings.aimBirds.load(),
                    kSettings.aimPriorityBirds.load());
    case Kind::Vehicle:
      return Ranked(kSettings.aimVehicles.load(),
                    kSettings.aimPriorityVehicles.load());
    case Kind::Self:
    case Kind::Ally:
    case Kind::Civilian:
    case Kind::Prop:
    default:
      return std::nullopt;
  }
}

}  // namespace afw::esp
