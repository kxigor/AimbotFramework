#include <afw/abi/abi.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/process/memory.hpp>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] bool IsTemplateSpawn(const Vec3& position) noexcept {
  return std::fabs(position.x - kTemplateCoordinate) <= kTemplateTolerance &&
         std::fabs(position.y - kTemplateCoordinate) <= kTemplateTolerance;
}

using DescriptorFn = void*(AFW_ENGINE_CALL*)(void* self);
using AiComponentFn = void*(AFW_ENGINE_CALL*)(void* self);

struct ClassVerdict {
  std::uintptr_t vtable{};
  bool isEntity{};
};

struct ClassCacheState {
  std::array<ClassVerdict, kClassCacheSlots> slots{};
  std::uint32_t used{};
};

[[nodiscard]] ClassCacheState& EntityCache() noexcept {
  static ClassCacheState cache{};
  return cache;
}

[[nodiscard]] ClassCacheState& PawnCache() noexcept {
  static ClassCacheState cache{};
  return cache;
}

[[nodiscard]] ClassCacheState& PawnAgentCache() noexcept {
  static ClassCacheState cache{};
  return cache;
}

[[nodiscard]] ClassCacheState& AnimalAgentCache() noexcept {
  static ClassCacheState cache{};
  return cache;
}

[[nodiscard]] ClassCacheState& VehicleAgentCache() noexcept {
  static ClassCacheState cache{};
  return cache;
}

[[nodiscard]] ClassCacheState& SimpleAnimalAgentCache() noexcept {
  static ClassCacheState cache{};
  return cache;
}

[[nodiscard]] std::optional<std::uint32_t> OwnHash(
    std::uintptr_t classDef) noexcept {
  const std::expected<std::uint32_t, std::error_code> kAncestors{
      process::ReadAt<std::uint32_t>(classDef + kClassDefAncestors)};
  if (!kAncestors || *kAncestors == 0 || *kAncestors > kMaxAncestors) {
    return std::nullopt;
  }

  const std::expected<std::uint32_t, std::error_code> kHash{
      process::ReadAt<std::uint32_t>(
          classDef + kClassDefHashes +
          ((*kAncestors - 1) * sizeof(std::uint32_t)))};
  if (!kHash) {
    return std::nullopt;
  }

  return *kHash;
}

[[nodiscard]] bool DescribesAnEntity(std::uintptr_t descriptor,
                                     std::uint32_t wanted) noexcept {
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(descriptor + kClassDefAncestors)};
  if (!kCount || *kCount == 0 || *kCount > kMaxAncestors) {
    return false;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::expected<std::uint32_t, std::error_code> kHash{
        process::ReadAt<std::uint32_t>(descriptor + kClassDefHashes +
                                       (index * sizeof(std::uint32_t)))};
    if (kHash && *kHash == wanted) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] std::optional<bool> AskTheEngine(
    std::uintptr_t address, std::uintptr_t vtable,
    std::uintptr_t classDef) noexcept {
  const std::uintptr_t kBase{Resolve(kGhidraImageBase)};
  if (kBase == 0) {
    return std::nullopt;
  }

  const std::optional<std::uint32_t> kWanted{OwnHash(Resolve(classDef))};
  if (!kWanted) {
    return false;
  }

  if (vtable < kBase || vtable >= kBase + kMaxModuleSpan) {
    return false;
  }

  const std::expected<std::uintptr_t, std::error_code> kSlot{
      process::ReadPointerAs<EnginePtr>(
          vtable + (kClassDescriptorSlot * sizeof(EnginePtr)))};
  if (!kSlot || *kSlot < kBase || *kSlot >= kBase + kMaxModuleSpan) {
    return false;
  }

  const DescriptorFn kDescriptorOf{std::bit_cast<DescriptorFn>(*kSlot)};
  const void* const kDescriptor{kDescriptorOf(std::bit_cast<void*>(address))};
  if (kDescriptor == nullptr) {
    return false;
  }

  return DescribesAnEntity(std::bit_cast<std::uintptr_t>(kDescriptor),
                           *kWanted);
}

[[nodiscard]] std::expected<Matrix4, std::error_code> ReadWorldMatrix(
    std::uintptr_t address) noexcept {
  const std::expected<Matrix4, std::error_code> kOwn{
      process::ReadAt<Matrix4>(address + kEntityTransform)};
  if (kOwn && kOwn->IsRigid(kUnitTolerance, kAxisTolerance)) {
    return *kOwn;
  }

  const std::optional<Matrix4> kFromComponent{
      ReadComponentWorldMatrix(address)};
  if (kFromComponent) {
    return *kFromComponent;
  }

  return kOwn;
}

[[nodiscard]] bool IsPlausibleArmy(std::int32_t army) noexcept {
  return army == kNoArmy || (army >= 0 && army <= kMaxArmy);
}

[[nodiscard]] bool IsPlausibleFaction(std::int32_t faction) noexcept {
  return faction >= kNoFaction && faction <= kMaxFaction;
}

[[nodiscard]] bool IsPlausibleAgentType(std::uint32_t agentType) noexcept {
  return agentType <= kMaxAgentType;
}

[[nodiscard]] bool IsOfClass(std::uintptr_t address, std::uintptr_t classDef,
                             ClassCacheState& cache) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kVtable{
      process::ReadPointerAs<EnginePtr>(address)};
  if (!kVtable || *kVtable == 0) {
    return false;
  }

  for (std::uint32_t index{0}; index < cache.used; ++index) {
    if (cache.slots.at(index).vtable == *kVtable) {
      return cache.slots.at(index).isEntity;
    }
  }

  const std::optional<bool> kVerdict{AskTheEngine(address, *kVtable, classDef)};
  if (!kVerdict) {
    return true;
  }

  if (cache.used < kClassCacheSlots) {
    cache.slots.at(cache.used) =
        ClassVerdict{.vtable = *kVtable, .isEntity = *kVerdict};
    ++cache.used;
  }

  return *kVerdict;
}

[[nodiscard]] std::optional<std::uintptr_t> AgentFromComponents(
    std::uintptr_t address) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kArray{
      process::ReadPointerAs<EnginePtr>(address + kEntityComponentArray)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(address + kEntityComponentCount)};
  if (!kArray || !kCount || *kArray == 0 || *kCount > kMaxComponents) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::expected<std::uintptr_t, std::error_code> kComponent{
        process::ReadPointerAs<EnginePtr>(*kArray +
                                          (index * sizeof(EnginePtr)))};
    if (!kComponent || *kComponent == 0) {
      continue;
    }

    const std::expected<std::uintptr_t, std::error_code> kAgent{
        process::ReadPointerAs<EnginePtr>(*kComponent + kComponentAgent)};
    if (kAgent && *kAgent != 0) {
      return *kAgent;
    }
  }

  return std::nullopt;
}

[[nodiscard]] bool WouldBuildHashTable(std::uintptr_t address) noexcept {
  const std::expected<std::uint32_t, std::error_code> kFlags{
      process::ReadAt<std::uint32_t>(address + kEntityFlags)};
  if (!kFlags || (*kFlags & kEntityHashTableFlag) == 0) {
    return false;
  }

  const std::expected<std::uintptr_t, std::error_code> kTable{
      process::ReadPointerAs<EnginePtr>(address + kEntityHashTable)};

  return !kTable || *kTable == 0;
}

[[nodiscard]] std::optional<std::uintptr_t> FindAgent(
    std::uintptr_t address) noexcept {
  const std::uintptr_t kAccessor{Resolve(kGetAiComponent)};
  if (kAccessor == 0) {
    return AgentFromComponents(address);
  }
  if (!IsEntityClass(address) || WouldBuildHashTable(address)) {
    return std::nullopt;
  }

  const AiComponentFn kGetComponent{std::bit_cast<AiComponentFn>(kAccessor)};
  const void* const kComponent{kGetComponent(std::bit_cast<void*>(address))};
  if (kComponent == nullptr) {
    return std::nullopt;
  }

  const std::expected<std::uintptr_t, std::error_code> kAgent{
      process::ReadPointerAs<EnginePtr>(
          std::bit_cast<std::uintptr_t>(kComponent) + kComponentAgent)};
  if (!kAgent || *kAgent == 0) {
    return std::nullopt;
  }

  return *kAgent;
}

[[nodiscard]] std::optional<AgentKind> AgentClassOf(
    std::uintptr_t agent) noexcept {
  if (IsOfClass(agent, kPawnAgentClassDef, PawnAgentCache())) {
    return AgentKind::Pawn;
  }
  if (IsOfClass(agent, kAnimalAgentClassDef, AnimalAgentCache())) {
    return AgentKind::Animal;
  }
  if (IsOfClass(agent, kVehicleAgentClassDef, VehicleAgentCache())) {
    return AgentKind::Vehicle;
  }
  if (IsOfClass(agent, kSimpleAnimalAgentClassDef, SimpleAnimalAgentCache())) {
    return AgentKind::SimpleAnimal;
  }

  return std::nullopt;
}

void ReadPawnAgentFields(std::uintptr_t agent, PawnInfo& pawn) noexcept {
  const std::expected<std::int32_t, std::error_code> kArmy{
      process::ReadAt<std::int32_t>(agent + kAgentArmy)};
  const std::expected<std::int32_t, std::error_code> kFaction{
      process::ReadAt<std::int32_t>(agent + kAgentFaction)};
  const std::expected<std::uint32_t, std::error_code> kType{
      process::ReadAt<std::uint32_t>(agent + kAgentType)};
  if (kArmy && IsPlausibleArmy(*kArmy)) {
    pawn.army = *kArmy;
  }
  if (kFaction && IsPlausibleFaction(*kFaction)) {
    pawn.faction = *kFaction;
  }
  if (kType && IsPlausibleAgentType(*kType)) {
    pawn.agentType = *kType;
  }
  if (pawn.agentType == kAgentCompanion) {
    return;
  }

  const std::uint8_t kFlag{
      process::ReadAt<std::uint8_t>(agent + kAgentIsDead).value_or(0)};
  const std::int32_t kState{
      process::ReadAt<std::int32_t>(agent + kAgentArmyMemberState).value_or(0)};
  pawn.dead = kFlag != 0 || kState == kArmyMemberStateDead;
}

[[nodiscard]] std::optional<PawnInfo> ReadAgent(std::uintptr_t agent,
                                                AgentKind kind) noexcept {
  if (kind == AgentKind::SimpleAnimal) {
    return PawnInfo{.kind = kind};
  }

  const std::uintptr_t kBlock{agent + (kind == AgentKind::Animal
                                           ? kAnimalAgentArmyBlock
                                           : kPawnAgentArmyBlock)};
  const std::expected<std::uint8_t, std::error_code> kOwnBit{
      process::ReadAt<std::uint8_t>(kBlock)};
  if (!kOwnBit) {
    return std::nullopt;
  }

  PawnInfo pawn{};
  pawn.kind = kind;
  pawn.ownArmyBit = *kOwnBit;
  pawn.hostileMask =
      process::ReadAt<std::uint8_t>(kBlock + kArmyBlockHostile).value_or(0);
  pawn.reverseHostileMask =
      process::ReadAt<std::uint8_t>(kBlock + kArmyBlockReverseHostile)
          .value_or(0);

  if (kind == AgentKind::Animal) {
    pawn.dead =
        process::ReadAt<std::uint8_t>(agent + kAnimalAgentIsDead).value_or(0) !=
        0;
  }
  if (kind != AgentKind::Pawn) {
    return pawn;
  }

  ReadPawnAgentFields(agent, pawn);

  return pawn;
}

[[nodiscard]] bool IsHumanoid(const Aabb& box) noexcept {
  return box.max.z > kHumanoidMinHeight && box.max.z < kHumanoidMaxHeight &&
         std::fabs(box.min.z) < kHumanoidFootTolerance &&
         std::fabs(box.max.x) < kHumanoidMaxWidth &&
         std::fabs(box.max.y) < kHumanoidMaxWidth;
}

}  // namespace

bool IsEntityClass(std::uintptr_t address) noexcept {
  return IsOfClass(address, kEntityClassDef, EntityCache());
}

bool IsPawnClass(std::uintptr_t address) noexcept {
  return IsOfClass(address, kPawnClassDef, PawnCache());
}

std::optional<EntityIdentity> ReadIdentity(std::uintptr_t address) noexcept {
  const std::uintptr_t kWanted{Resolve(kIdentityComponentVtable)};
  if (kWanted == 0) {
    return std::nullopt;
  }

  const std::expected<std::uintptr_t, std::error_code> kArray{
      process::ReadPointerAs<EnginePtr>(address + kEntityComponentArray)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(address + kEntityComponentCount)};
  if (!kArray || !kCount || *kArray == 0 || *kCount > kMaxComponents) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::expected<std::uintptr_t, std::error_code> kComponent{
        process::ReadPointerAs<EnginePtr>(*kArray +
                                          (index * sizeof(EnginePtr)))};
    if (!kComponent || *kComponent == 0) {
      continue;
    }

    const std::expected<std::uintptr_t, std::error_code> kVtable{
        process::ReadPointerAs<EnginePtr>(*kComponent)};
    if (!kVtable || *kVtable != kWanted) {
      continue;
    }

    const std::expected<std::uint32_t, std::error_code> kClassIndex{
        process::ReadAt<std::uint32_t>(*kComponent + kIdentityClassIndex)};
    const std::expected<std::uint32_t, std::error_code> kClassHash{
        process::ReadAt<std::uint32_t>(*kComponent + kIdentityClassHash)};
    if (!kClassIndex || !kClassHash || *kClassHash == kNoArchetype) {
      return std::nullopt;
    }

    return EntityIdentity{.classIndex = *kClassIndex, .classHash = *kClassHash};
  }

  return std::nullopt;
}

std::optional<std::uintptr_t> OwnerOfComponent(
    std::uintptr_t address) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kHandle{
      process::ReadPointerAs<EnginePtr>(address + kComponentOwnerHandle)};
  if (!kHandle || *kHandle == 0) {
    return std::nullopt;
  }

  const std::expected<std::uintptr_t, std::error_code> kOwner{
      process::ReadPointerAs<EnginePtr>(*kHandle + kHandleEntity)};
  if (!kOwner || *kOwner == 0 || *kOwner == address) {
    return std::nullopt;
  }
  if (!IsEntityClass(*kOwner)) {
    return std::nullopt;
  }

  return *kOwner;
}

std::optional<Matrix4> ReadComponentWorldMatrix(
    std::uintptr_t address) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kArray{
      process::ReadPointerAs<EnginePtr>(address + kEntityComponentArray)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(address + kEntityComponentCount)};
  if (!kArray || !kCount || *kArray == 0 || *kCount > kMaxComponents) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::expected<std::uintptr_t, std::error_code> kComponent{
        process::ReadPointerAs<EnginePtr>(*kArray +
                                          (index * sizeof(EnginePtr)))};
    if (!kComponent || *kComponent == 0) {
      continue;
    }

    const std::expected<Matrix4, std::error_code> kMatrix{
        process::ReadAt<Matrix4>(*kComponent + kComponentWorldMatrix)};
    if (kMatrix && kMatrix->IsRigid(kUnitTolerance, kAxisTolerance)) {
      return *kMatrix;
    }
  }

  return std::nullopt;
}

std::expected<RawEntity, std::error_code> ReadEntity(
    std::uintptr_t address) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kVtable{
      process::ReadPointerAs<EnginePtr>(address)};
  if (!kVtable) {
    return std::unexpected{kVtable.error()};
  }

  const std::expected<Matrix4, std::error_code> kTransform{
      ReadWorldMatrix(address)};
  if (!kTransform) {
    return std::unexpected{kTransform.error()};
  }

  const std::expected<Aabb, std::error_code> kBox{
      process::ReadAt<Aabb>(address + kEntityAabb)};
  if (!kBox) {
    return std::unexpected{kBox.error()};
  }

  return RawEntity{.address = address,
                   .vtable = *kVtable,
                   .transform = *kTransform,
                   .box = *kBox};
}

std::expected<EntityInfo, Rejection> Classify(const RawEntity& raw) noexcept {
  if (raw.vtable == 0) {
    return std::unexpected{Rejection::NoVtable};
  }
  if (!raw.transform.IsRigid(kUnitTolerance, kAxisTolerance)) {
    return std::unexpected{Rejection::NotRigid};
  }
  if (!raw.box.IsValid(kMaxBoxSpan)) {
    return std::unexpected{Rejection::BadBox};
  }

  const Vec3 kPosition{raw.transform.Translation()};
  if (!kPosition.IsFinite()) {
    return std::unexpected{Rejection::NonFinite};
  }
  if (!kPosition.IsWithin(kMaxWorldCoordinate)) {
    return std::unexpected{Rejection::BeyondTheWorld};
  }
  if (IsTemplateSpawn(kPosition)) {
    return std::unexpected{Rejection::TemplateSpawn};
  }

  return EntityInfo{.address = raw.address,
                    .vtable = raw.vtable,
                    .transform = raw.transform,
                    .box = raw.box,
                    .pawn = std::nullopt,
                    .humanoid = IsHumanoid(raw.box)};
}

std::optional<PawnInfo> ReadPawn(std::uintptr_t address) noexcept {
  const std::optional<std::uintptr_t> kAgent{FindAgent(address)};
  if (!kAgent) {
    return std::nullopt;
  }

  const std::optional<AgentKind> kKind{AgentClassOf(*kAgent)};
  if (!kKind) {
    return std::nullopt;
  }

  return ReadAgent(*kAgent, *kKind);
}

}  // namespace afw::games::fc3
