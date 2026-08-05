#include <afw/abi/abi.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/class_registry.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

namespace {

using DescriptorFn = void*(AFW_ENGINE_CALL*)(void* self);

[[nodiscard]] std::optional<std::uint32_t> OwnHash(
    std::uintptr_t classDef) noexcept {
  const Engine kClass{classDef};
  const std::optional<std::uint32_t> kAncestors{
      kClass.CountAtMost(kClassDefAncestors, kMaxAncestors)};
  if (!kAncestors || *kAncestors == 0) {
    return std::nullopt;
  }

  return kClass.Field<std::uint32_t>(
      kClassDefHashes + ((*kAncestors - 1) * sizeof(std::uint32_t)));
}

[[nodiscard]] bool DescribesTheClass(std::uintptr_t descriptor,
                                     std::uint32_t wanted) noexcept {
  const Engine kDescriptor{descriptor};
  const std::optional<std::uint32_t> kCount{
      kDescriptor.CountAtMost(kClassDefAncestors, kMaxAncestors)};
  if (!kCount || *kCount == 0) {
    return false;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::optional<std::uint32_t> kHash{kDescriptor.Field<std::uint32_t>(
        kClassDefHashes + (index * sizeof(std::uint32_t)))};
    if (kHash && *kHash == wanted) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] bool TakesOnlyThis(std::uintptr_t function) noexcept {
  std::uintptr_t entry{function};
  for (std::uint32_t hop{0}; hop < kMaxThunkHops; ++hop) {
    const std::optional<std::uint8_t> kOpcode{
        Engine{entry}.Field<std::uint8_t>(0)};
    if (!kOpcode || *kOpcode != kRelativeJump) {
      break;
    }

    const std::optional<std::int32_t> kOffset{
        Engine{entry}.Field<std::int32_t>(1)};
    if (!kOffset) {
      return false;
    }
    entry = entry + kRelativeJumpSize + static_cast<std::uintptr_t>(*kOffset);
  }

  const Engine kEntry{entry};
  for (std::uint32_t step{0}; step < kGetterScanBytes; ++step) {
    const std::optional<std::uint8_t> kByte{kEntry.Field<std::uint8_t>(step)};
    if (!kByte || *kByte == kReturnWithOperand) {
      return false;
    }
    if (*kByte == kReturn) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] bool WithinTheModule(std::uintptr_t address,
                                   std::uintptr_t base) noexcept {
  return address >= base && address < base + kMaxModuleSpan;
}

[[nodiscard]] std::optional<bool> AskTheEngine(
    std::uintptr_t address, std::uintptr_t vtable,
    std::uintptr_t classDef) noexcept {
  const std::uintptr_t kBase{Resolve(kGhidraImageBase)};
  if (kBase == 0) {
    return std::nullopt;
  }

  const std::optional<std::uint32_t> kWanted{OwnHash(Resolve(classDef))};
  if (!kWanted || !WithinTheModule(vtable, kBase)) {
    return false;
  }

  const std::optional<std::uintptr_t> kSlot{
      Engine{vtable}.Pointer(kClassDescriptorSlot * sizeof(EnginePtr))};
  if (!kSlot || !WithinTheModule(*kSlot, kBase) || !TakesOnlyThis(*kSlot)) {
    return false;
  }

  const DescriptorFn kDescriptorOf{std::bit_cast<DescriptorFn>(*kSlot)};
  const void* const kDescriptor{kDescriptorOf(std::bit_cast<void*>(address))};
  if (kDescriptor == nullptr) {
    return false;
  }

  return DescribesTheClass(std::bit_cast<std::uintptr_t>(kDescriptor),
                           *kWanted);
}

[[nodiscard]] constexpr std::uintptr_t ClassDefOf(AgentKind kind) noexcept {
  switch (kind) {
    case AgentKind::Pawn:
      return kPawnAgentClassDef;
    case AgentKind::Animal:
      return kAnimalAgentClassDef;
    case AgentKind::Vehicle:
      return kVehicleAgentClassDef;
    case AgentKind::SimpleAnimal:
    default:
      return kSimpleAnimalAgentClassDef;
  }
}

}  // namespace

ClassProbe::ClassProbe(std::uintptr_t classDef) noexcept
    : classDef_{classDef} {}

bool ClassProbe::Matches(std::uintptr_t address) noexcept {
  const std::optional<std::uintptr_t> kVtable{Engine{address}.Pointer(0)};
  if (!kVtable) {
    return false;
  }

  for (std::uint32_t index{0}; index < used_; ++index) {
    if (slots_.at(index).vtable == *kVtable) {
      return slots_.at(index).matches;
    }
  }

  const std::optional<bool> kVerdict{
      AskTheEngine(address, *kVtable, classDef_)};
  if (!kVerdict) {
    return true;
  }

  if (used_ < kClassCacheSlots) {
    slots_.at(used_) = Verdict{.vtable = *kVtable, .matches = *kVerdict};
    ++used_;
  }

  return *kVerdict;
}

ClassProbe& EntityProbe() noexcept {
  static ClassProbe probe{kEntityClassDef};

  return probe;
}

ClassProbe& AgentProbe(AgentKind kind) noexcept {
  static std::array<ClassProbe, kAgentKindCount> probes{
      ClassProbe{ClassDefOf(AgentKind::Pawn)},
      ClassProbe{ClassDefOf(AgentKind::Animal)},
      ClassProbe{ClassDefOf(AgentKind::SimpleAnimal)},
      ClassProbe{ClassDefOf(AgentKind::Vehicle)}};

  return probes.at(static_cast<std::size_t>(kind));
}

bool IsEntityClass(std::uintptr_t address) noexcept {
  return EntityProbe().Matches(address);
}

}  // namespace afw::games::fc3
