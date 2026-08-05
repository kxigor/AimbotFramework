#include <afw/games/fc3/internal/agent.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] bool IsPlausibleArmy(std::int32_t army) noexcept {
  return army == kNoArmy || (army >= 0 && army <= kMaxArmy);
}

[[nodiscard]] bool IsPlausibleFaction(std::int32_t faction) noexcept {
  return faction >= kNoFaction && faction <= kMaxFaction;
}

// A field that reads back nonsense costs that field and not the record: what
// the record exists for is the agent's class, and that was established before
// any of these was read.
void ReadPawnAgentFields(std::uintptr_t agent, PawnInfo& pawn) noexcept {
  const Engine kAgent{agent};
  const std::optional<std::int32_t> kArmy{
      kAgent.Field<std::int32_t>(kAgentArmy)};
  const std::optional<std::int32_t> kFaction{
      kAgent.Field<std::int32_t>(kAgentFaction)};
  const std::optional<std::uint32_t> kType{
      kAgent.Field<std::uint32_t>(kAgentType)};
  if (kArmy && IsPlausibleArmy(*kArmy)) {
    pawn.army = *kArmy;
  }
  if (kFaction && IsPlausibleFaction(*kFaction)) {
    pawn.faction = *kFaction;
  }
  if (kType && *kType <= kMaxAgentType) {
    pawn.agentType = *kType;
  }
  if (pawn.agentType == kAgentCompanion) {
    return;
  }

  pawn.dead = kAgent.FieldOr<std::uint8_t>(kAgentIsDead, 0) != 0 ||
              kAgent.FieldOr<std::int32_t>(kAgentArmyMemberState, 0) ==
                  kArmyMemberStateDead;
}

[[nodiscard]] std::uintptr_t ArmyBlockOf(std::uintptr_t agent,
                                         AgentKind kind) noexcept {
  return agent + (kind == AgentKind::Animal ? kAnimalAgentArmyBlock
                                            : kPawnAgentArmyBlock);
}

}  // namespace

std::optional<PawnInfo> ReadAgent(std::uintptr_t agent,
                                  AgentKind kind) noexcept {
  if (kind == AgentKind::SimpleAnimal) {
    return PawnInfo{.kind = kind,
                    .dead = Engine{agent}.FieldOr<std::uint8_t>(
                                kSimpleAnimalAgentIsDead, 0) != 0};
  }

  const Engine kBlock{ArmyBlockOf(agent, kind)};
  const std::optional<std::uint8_t> kOwnBit{kBlock.Field<std::uint8_t>(0)};
  if (!kOwnBit) {
    return std::nullopt;
  }

  PawnInfo pawn{};
  pawn.kind = kind;
  pawn.ownArmyBit = *kOwnBit;
  pawn.hostileMask = kBlock.FieldOr<std::uint8_t>(kArmyBlockHostile, 0);
  pawn.reverseHostileMask =
      kBlock.FieldOr<std::uint8_t>(kArmyBlockReverseHostile, 0);

  if (kind == AgentKind::Animal) {
    pawn.dead = Engine{agent}.FieldOr<std::uint8_t>(kAnimalAgentIsDead, 0) != 0;
  }
  if (kind != AgentKind::Pawn) {
    return pawn;
  }

  ReadPawnAgentFields(agent, pawn);

  return pawn;
}

}  // namespace afw::games::fc3
