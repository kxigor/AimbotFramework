#include <afw/config/config.hpp>
#include <afw/esp/internal/palette.hpp>
#include <afw/games/fc3/internal/kind.hpp>
#include <cstdint>

namespace afw::esp {

namespace {

using games::fc3::Kind;

constexpr std::uint32_t kPropColour{0xC000FF40};
constexpr std::uint32_t kDeadColour{0x80606060};
constexpr std::uint32_t kEnemyColour{0xFF3030FF};
constexpr std::uint32_t kAllyColour{0xFF30FF30};
constexpr std::uint32_t kCivilianColour{0xFFA0FFA0};
constexpr std::uint32_t kAnimalColour{0xFF7070A0};
constexpr std::uint32_t kBirdColour{0xFFC0C070};
constexpr std::uint32_t kPredatorColour{0xFF2060FF};
constexpr std::uint32_t kVehicleColour{0xFF40C0FF};

}  // namespace

std::uint32_t ColourOf(const games::fc3::EntityInfo& entity,
                       Kind kind) noexcept {
  if (entity.pawn && entity.pawn->dead) {
    return kDeadColour;
  }

  switch (kind) {
    case Kind::Self:
    case Kind::Enemy:
      return kEnemyColour;
    case Kind::Ally:
      return kAllyColour;
    case Kind::Civilian:
      return kCivilianColour;
    case Kind::Predator:
      return kPredatorColour;
    case Kind::Animal:
      return kAnimalColour;
    case Kind::Bird:
      return kBirdColour;
    case Kind::Vehicle:
      return kVehicleColour;
    case Kind::Prop:
    default:
      return kPropColour;
  }
}

bool Shown(Kind kind) noexcept {
  const config::Settings& kSettings{config::Current()};
  switch (kind) {
    case Kind::Enemy:
      return kSettings.espEnemies.load();
    case Kind::Ally:
      return kSettings.fc3.espAllies.load();
    case Kind::Civilian:
      return kSettings.fc3.espCivilians.load();
    case Kind::Predator:
    case Kind::Animal:
      return kSettings.fc3.espAnimals.load();
    case Kind::Bird:
      return kSettings.fc3.espBirds.load();
    case Kind::Vehicle:
      return kSettings.fc3.espVehicles.load();
    case Kind::Prop:
      return kSettings.espProps.load();
    case Kind::Self:
    default:
      return false;
  }
}

}  // namespace afw::esp
