#include <gtest/gtest.h>

#include <afw/esp/internal/classify.hpp>
#include <afw/games/fc3/fc3.hpp>

namespace {

using afw::esp::Kind;
using afw::games::fc3::AgentKind;
using afw::games::fc3::PawnInfo;

// Every value below was read out of the running game, so these cases pin
// measured behaviour rather than an invented scheme. The player carries army 2
// with a hostile mask of 0x03, which is hostile to armies 0 and 1 -- pirates
// are army 1, and that is why they come out red without anything here saying
// "pirates are enemies".
[[nodiscard]] PawnInfo Player() {
  return PawnInfo{.kind = AgentKind::Pawn,
                  .army = 2,
                  .faction = afw::games::fc3::kFactionUnknown,
                  .agentType = afw::games::fc3::kAgentPlayer,
                  .ownArmyBit = 0x04,
                  .hostileMask = 0x03,
                  .reverseHostileMask = 0x00,
                  .dead = false};
}

[[nodiscard]] PawnInfo Pirate() {
  return PawnInfo{.kind = AgentKind::Pawn,
                  .army = 1,
                  .faction = afw::games::fc3::kFactionPirate,
                  .agentType = afw::games::fc3::kAgentUnknown,
                  .ownArmyBit = 0x02,
                  .hostileMask = 0x03,
                  .reverseHostileMask = 0x00,
                  .dead = false};
}

[[nodiscard]] PawnInfo Rakyat() {
  return PawnInfo{.kind = AgentKind::Pawn,
                  .army = 4,
                  .faction = afw::games::fc3::kFactionRebel,
                  .agentType = afw::games::fc3::kAgentUnknown,
                  .ownArmyBit = 0x10,
                  .hostileMask = 0x03,
                  .reverseHostileMask = 0x00,
                  .dead = false};
}

[[nodiscard]] PawnInfo Civilian() {
  return PawnInfo{.kind = AgentKind::Pawn,
                  .army = 5,
                  .faction = afw::games::fc3::kFactionUnknown,
                  .agentType = afw::games::fc3::kAgentUnknown,
                  .ownArmyBit = 0x20,
                  .hostileMask = 0x03,
                  .reverseHostileMask = 0x00,
                  .dead = false};
}

}  // namespace

TEST(EspClassifyUnit, APirateIsAnEnemyByFaction) {
  const PawnInfo kSelf{Player()};

  EXPECT_EQ(afw::esp::KindOfPawn(Pirate(), &kSelf), Kind::Enemy);
}

TEST(EspClassifyUnit, ARakyatIsAnAllyByFaction) {
  const PawnInfo kSelf{Player()};

  EXPECT_EQ(afw::esp::KindOfPawn(Rakyat(), &kSelf), Kind::Ally);
}

// Army 5 is a real army, and an earlier version answered "has an army, so it is
// an enemy" and painted every civilian red. The engine's predicate says
// otherwise: 0x20 & 0x03 is zero.
TEST(EspClassifyUnit, ACivilianWithAnArmyIsStillNotAnEnemy) {
  const PawnInfo kSelf{Player()};

  EXPECT_FALSE(afw::esp::ArmiesHostile(kSelf, Civilian()));
  EXPECT_EQ(afw::esp::KindOfPawn(Civilian(), &kSelf), Kind::Civilian);
}

TEST(EspClassifyUnit, HostilityIsDecidedByTheMasksNotTheArmyNumber) {
  const PawnInfo kSelf{Player()};

  EXPECT_TRUE(afw::esp::ArmiesHostile(kSelf, Pirate()));
}

TEST(EspClassifyUnit, TheAgentTypeOutranksTheFaction) {
  const PawnInfo kSelf{Player()};

  PawnInfo companion{Pirate()};
  companion.agentType = afw::games::fc3::kAgentCompanion;
  EXPECT_EQ(afw::esp::KindOfPawn(companion, &kSelf), Kind::Ally);

  PawnInfo civilian{Pirate()};
  civilian.agentType = afw::games::fc3::kAgentCivilian;
  EXPECT_EQ(afw::esp::KindOfPawn(civilian, &kSelf), Kind::Civilian);

  EXPECT_EQ(afw::esp::KindOfPawn(Player(), &kSelf), Kind::Self);
}

// The class of the agent settles it before any field is read, which is what
// makes a boar a boar however oddly its bounding box is shaped.
TEST(EspClassifyUnit, TheAgentClassOutranksEverything) {
  const PawnInfo kSelf{Player()};

  PawnInfo animal{Pirate()};
  animal.kind = AgentKind::Animal;
  animal.ownArmyBit = 0x08;
  animal.hostileMask = 0x37;
  animal.reverseHostileMask = 0x00;
  EXPECT_EQ(afw::esp::KindOfPawn(animal, &kSelf), Kind::Animal);

  PawnInfo bird{Pirate()};
  bird.kind = AgentKind::SimpleAnimal;
  EXPECT_EQ(afw::esp::KindOfPawn(bird, &kSelf), Kind::Bird);

  PawnInfo vehicle{Pirate()};
  vehicle.kind = AgentKind::Vehicle;
  EXPECT_EQ(afw::esp::KindOfPawn(vehicle, &kSelf), Kind::Vehicle);
}

TEST(EspClassifyUnit, AnAnimalHostileToThePlayerIsAPredator) {
  const PawnInfo kSelf{Player()};

  PawnInfo predator{Pirate()};
  predator.kind = AgentKind::Animal;
  predator.reverseHostileMask = kSelf.ownArmyBit;

  EXPECT_EQ(afw::esp::KindOfPawn(predator, &kSelf), Kind::Predator);
}

// Without a player there is nobody to be hostile towards, and answering "enemy"
// there once painted a whole village red.
TEST(EspClassifyUnit, WithoutAPlayerNobodyBecomesAnEnemyByTheMasks) {
  PawnInfo stranger{Civilian()};
  stranger.faction = afw::games::fc3::kFactionUnknown;

  EXPECT_EQ(afw::esp::KindOfPawn(stranger, nullptr), Kind::Civilian);
}

TEST(EspClassifyUnit, AnEntityWithoutAPawnRecordIsAProp) {
  const afw::games::fc3::EntityInfo kEntity{};

  EXPECT_EQ(afw::esp::KindOf(kEntity, nullptr), Kind::Prop);
}

TEST(EspClassifyUnit, TheDeadKeepTheirOwnColourWhateverTheyWere) {
  afw::games::fc3::EntityInfo entity{};
  entity.pawn = Pirate();
  entity.pawn->dead = true;

  const std::uint32_t kDead{afw::esp::ColourOf(entity, Kind::Enemy)};
  entity.pawn->dead = false;

  EXPECT_NE(kDead, afw::esp::ColourOf(entity, Kind::Enemy));
}

TEST(EspClassifyUnit, FindSelfPicksThePlayerAndNotTheNearestPawn) {
  static afw::games::fc3::EntitySnapshot snapshot{};
  snapshot.count = 3;
  snapshot.items.at(0).pawn = Pirate();
  snapshot.items.at(1).pawn = Player();
  snapshot.items.at(2).pawn = Rakyat();

  const PawnInfo* const kSelf{afw::esp::FindSelf(snapshot)};

  ASSERT_NE(kSelf, nullptr);
  EXPECT_EQ(kSelf->agentType, afw::games::fc3::kAgentPlayer);
}

TEST(EspClassifyUnit, FindSelfIsNullWhenThePlayerIsAbsent) {
  static afw::games::fc3::EntitySnapshot snapshot{};
  snapshot.count = 1;
  snapshot.items.at(0).pawn = Pirate();

  EXPECT_EQ(afw::esp::FindSelf(snapshot), nullptr);
}
