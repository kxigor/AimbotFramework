#include <afw/games/fc3/internal/bones.hpp>
#include <afw/games/fc3/internal/class_registry.hpp>
#include <afw/games/fc3/internal/components.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] bool CarriesAPose(std::uintptr_t component) noexcept {
  const std::optional<BoneTable> kBones{BonesOf(component)};

  return kBones && kBones->count != 0;
}

}  // namespace

std::optional<EngineArray> Components(std::uintptr_t entity) noexcept {
  return EngineArray::At(entity, kEntityComponentArray, kEntityComponentCount,
                         kMaxComponents);
}

std::optional<RigidComponent> FindRigidComponent(std::uintptr_t address,
                                                 bool posed) noexcept {
  return FirstComponent(
      address, [posed](std::uintptr_t part) -> std::optional<RigidComponent> {
        if (posed && !CarriesAPose(part)) {
          return std::nullopt;
        }

        const std::optional<math::Matrix4> kMatrix{
            Engine{part}.Field<math::Matrix4>(kComponentWorldMatrix)};
        if (!kMatrix || !kMatrix->IsRigid(kUnitTolerance, kAxisTolerance)) {
          return std::nullopt;
        }

        return RigidComponent{.address = part, .transform = *kMatrix};
      });
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
