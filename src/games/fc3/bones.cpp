#include <afw/games/fc3/internal/bones.hpp>
#include <afw/games/fc3/internal/components.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

std::optional<BoneTable> BonesOf(std::uintptr_t component) noexcept {
  const Engine kComponent{component};
  const std::optional<std::uintptr_t> kBase{
      kComponent.Pointer(kComponentBoneArray)};
  const std::optional<std::uint32_t> kCount{
      kComponent.CountAtMost(kComponentBoneCount, kMaxBones)};
  if (!kBase || !kCount) {
    return std::nullopt;
  }

  return BoneTable{.base = *kBase, .count = *kCount};
}

namespace {

[[nodiscard]] std::optional<math::Vec3> ReadBonePosition(
    const RigidComponent& component, std::uint32_t key) noexcept {
  const std::optional<BoneTable> kTable{BonesOf(component.address)};
  if (!kTable) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < kTable->count; ++index) {
    const Engine kEntry{kTable->base + (index * kBoneStride)};
    const std::optional<std::uint32_t> kKey{
        kEntry.Field<std::uint32_t>(kBoneKey)};
    if (!kKey) {
      return std::nullopt;
    }
    if (*kKey != key) {
      continue;
    }

    const std::optional<math::Vec3> kLocal{
        kEntry.Field<math::Vec3>(kBonePosePosition)};
    if (!kLocal || !kLocal->IsFinite()) {
      return std::nullopt;
    }

    return component.transform.TransformPoint(*kLocal);
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<math::Vec3> BoneNear(
    const RigidComponent& component, std::uint32_t key,
    const math::Vec3& origin) noexcept {
  const std::optional<math::Vec3> kBone{ReadBonePosition(component, key)};
  if (!kBone || (*kBone - origin).Length() > kMaxHeadOffset) {
    return std::nullopt;
  }

  return kBone;
}

}  // namespace

BonePoints ReadBonePoints(std::uintptr_t address) noexcept {
  const std::optional<RigidComponent> kComponent{
      FindRigidComponent(address, true)};
  if (!kComponent) {
    return BonePoints{};
  }

  const math::Vec3 kOrigin{kComponent->transform.Translation()};
  const std::optional<math::Vec3> kHead{
      BoneNear(*kComponent, kHeadBoneKey, kOrigin)};
  std::optional<math::Vec3> body{BoneNear(*kComponent, kSpineBoneKey, kOrigin)};
  if (!body) {
    body = BoneNear(*kComponent, kSpineOneBoneKey, kOrigin);
  }
  if (!body) {
    body = kHead;
  }

  return BonePoints{.head = kHead, .body = body};
}

}  // namespace afw::games::fc3
