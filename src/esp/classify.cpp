#include <afw/esp/internal/classify.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <cstdint>

namespace afw::esp {

namespace {

// Packed as 0xAABBGGRR, which is what ImGui expects.
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

bool ArmiesHostile(const games::fc3::PawnInfo& self,
                   const games::fc3::PawnInfo& other) noexcept {
  return ((other.ownArmyBit & self.hostileMask) != 0) ||
         ((other.reverseHostileMask & self.ownArmyBit) != 0);
}

const games::fc3::PawnInfo* FindSelf(
    const games::fc3::EntitySnapshot& snapshot) noexcept {
  for (std::uint32_t index{0}; index < snapshot.count; ++index) {
    const games::fc3::EntityInfo& kEntity{snapshot.items.at(index)};
    if (kEntity.pawn && kEntity.pawn->agentType == games::fc3::kAgentPlayer) {
      return &*kEntity.pawn;
    }
  }

  return nullptr;
}

Kind KindOfPawn(const games::fc3::PawnInfo& pawn,
                const games::fc3::PawnInfo* self) noexcept {
  if (pawn.kind == games::fc3::AgentKind::Vehicle) {
    return Kind::Vehicle;
  }
  if (pawn.kind == games::fc3::AgentKind::SimpleAnimal) {
    return Kind::Bird;
  }
  if (pawn.kind == games::fc3::AgentKind::Animal) {
    return (self != nullptr && ArmiesHostile(*self, pawn)) ? Kind::Predator
                                                           : Kind::Animal;
  }
  if (pawn.agentType == games::fc3::kAgentPlayer) {
    return Kind::Self;
  }
  if (pawn.agentType == games::fc3::kAgentCompanion) {
    return Kind::Ally;
  }
  if (pawn.agentType == games::fc3::kAgentCivilian) {
    return Kind::Civilian;
  }
  if (pawn.faction == games::fc3::kFactionPirate ||
      pawn.faction == games::fc3::kFactionPrivateer) {
    return Kind::Enemy;
  }
  if (pawn.faction == games::fc3::kFactionRebel) {
    return Kind::Ally;
  }

  return (self != nullptr && ArmiesHostile(*self, pawn)) ? Kind::Enemy
                                                         : Kind::Civilian;
}

Kind KindOf(const games::fc3::EntityInfo& entity,
            const games::fc3::PawnInfo* self) noexcept {
  if (entity.pawn) {
    return KindOfPawn(*entity.pawn, self);
  }

  return Kind::Prop;
}

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

const char* KindName(Kind kind) noexcept {
  switch (kind) {
    case Kind::Self:
      return "self";
    case Kind::Enemy:
      return "enemy";
    case Kind::Ally:
      return "ally";
    case Kind::Civilian:
      return "civilian";
    case Kind::Predator:
      return "predator";
    case Kind::Animal:
      return "animal";
    case Kind::Bird:
      return "bird";
    case Kind::Vehicle:
      return "vehicle";
    case Kind::Prop:
    default:
      return "prop";
  }
}

}  // namespace afw::esp
