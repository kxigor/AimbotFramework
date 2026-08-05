#include <afw/games/fc3/internal/kind.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <afw/games/fc3/snapshot.hpp>
#include <cstdint>

namespace afw::games::fc3 {

bool ArmiesHostile(const PawnInfo& self, const PawnInfo& other) noexcept {
  return ((other.ownArmyBit & self.hostileMask) != 0) ||
         ((other.reverseHostileMask & self.ownArmyBit) != 0);
}

const EntityInfo* SelfEntity(const EntitySnapshot& snapshot) noexcept {
  for (const EntityInfo& kEntity : Entities(snapshot)) {
    if (kEntity.pawn && kEntity.pawn->agentType == kAgentPlayer) {
      return &kEntity;
    }
  }

  return nullptr;
}

const PawnInfo* FindSelf(const EntitySnapshot& snapshot) noexcept {
  const EntityInfo* const kSelf{SelfEntity(snapshot)};
  if (kSelf == nullptr || !kSelf->pawn) {
    return nullptr;
  }

  return &*kSelf->pawn;
}

Kind KindOfPawn(const PawnInfo& pawn, const PawnInfo* self) noexcept {
  if (pawn.kind == AgentKind::Vehicle) {
    return Kind::Vehicle;
  }
  if (pawn.kind == AgentKind::SimpleAnimal) {
    return Kind::Bird;
  }
  if (pawn.kind == AgentKind::Animal) {
    return (self != nullptr && ArmiesHostile(*self, pawn)) ? Kind::Predator
                                                           : Kind::Animal;
  }
  if (pawn.agentType == kAgentPlayer) {
    return Kind::Self;
  }
  if (pawn.agentType == kAgentCompanion) {
    return Kind::Ally;
  }
  if (pawn.agentType == kAgentCivilian) {
    return Kind::Civilian;
  }
  if (pawn.faction == kFactionPirate || pawn.faction == kFactionPrivateer) {
    return Kind::Enemy;
  }
  if (pawn.faction == kFactionRebel) {
    return Kind::Ally;
  }

  return (self != nullptr && ArmiesHostile(*self, pawn)) ? Kind::Enemy
                                                         : Kind::Civilian;
}

Kind KindOf(const EntityInfo& entity, const PawnInfo* self) noexcept {
  if (entity.pawn) {
    return KindOfPawn(*entity.pawn, self);
  }

  return Kind::Prop;
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

}  // namespace afw::games::fc3
