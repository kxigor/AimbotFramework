#include <afw/games/fc3/internal/components.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] std::optional<math::Vec3> ReadBonePosition(
    const RigidComponent& component, std::uint32_t key) noexcept {
  const Engine kComponent{component.address};
  const std::optional<std::uintptr_t> kBones{
      kComponent.Pointer(kComponentBoneArray)};
  const std::optional<std::uint32_t> kCount{
      kComponent.CountAtMost(kComponentBoneCount, kMaxBones)};
  if (!kBones || !kCount) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const Engine kEntry{*kBones + (index * kBoneStride)};
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
