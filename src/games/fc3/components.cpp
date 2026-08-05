#include <afw/games/fc3/internal/components.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] bool CarriesAPose(std::uintptr_t component) noexcept {
  const Engine kComponent{component};
  const std::optional<std::uintptr_t> kBones{
      kComponent.Pointer(kComponentBoneArray)};
  const std::optional<std::uint32_t> kCount{
      kComponent.CountAtMost(kComponentBoneCount, kMaxBones)};

  return kBones && kCount && *kCount != 0;
}

}  // namespace

std::optional<EngineArray> Components(std::uintptr_t entity) noexcept {
  return EngineArray::At(entity, kEntityComponentArray, kEntityComponentCount,
                         kMaxComponents);
}

std::optional<RigidComponent> FindRigidComponent(std::uintptr_t address,
                                                 bool posed) noexcept {
  const std::optional<EngineArray> kParts{Components(address)};
  if (!kParts) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < kParts->Size(); ++index) {
    const std::optional<std::uintptr_t> kPart{kParts->Slot(index)};
    if (!kPart || (posed && !CarriesAPose(*kPart))) {
      continue;
    }

    const std::optional<math::Matrix4> kMatrix{
        Engine{*kPart}.Field<math::Matrix4>(kComponentWorldMatrix)};
    if (kMatrix && kMatrix->IsRigid(kUnitTolerance, kAxisTolerance)) {
      return RigidComponent{.address = *kPart, .transform = *kMatrix};
    }
  }

  return std::nullopt;
}

std::optional<std::uintptr_t> OwnerOfComponent(
    std::uintptr_t address) noexcept {
  const std::optional<std::uintptr_t> kHandle{
      Engine{address}.Pointer(kComponentOwnerHandle)};
  if (!kHandle) {
    return std::nullopt;
  }

  const std::optional<std::uintptr_t> kOwner{
      Engine{*kHandle}.Pointer(kHandleEntity)};
  if (!kOwner || *kOwner == address || !IsEntityClass(*kOwner)) {
    return std::nullopt;
  }

  return kOwner;
}

std::optional<math::Matrix4> ReadComponentWorldMatrix(
    std::uintptr_t address) noexcept {
  const std::optional<RigidComponent> kComponent{
      FindRigidComponent(address, false)};
  if (!kComponent) {
    return std::nullopt;
  }

  return kComponent->transform;
}

}  // namespace afw::games::fc3
