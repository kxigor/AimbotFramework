#include <afw/games/fc3/internal/components.hpp>
#include <afw/games/fc3/internal/entity.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>

namespace afw::games::fc3 {

namespace {

// Template and system entities all sit at one coordinate with an identity
// transform, so they pass every other test a real entity passes.
[[nodiscard]] bool IsTemplateSpawn(const math::Vec3& position) noexcept {
  return std::fabs(position.x - kTemplateCoordinate) <= kTemplateTolerance &&
         std::fabs(position.y - kTemplateCoordinate) <= kTemplateTolerance;
}

[[nodiscard]] bool IsHumanoid(const math::Aabb& box) noexcept {
  return box.max.z > kHumanoidMinHeight && box.max.z < kHumanoidMaxHeight &&
         std::fabs(box.min.z) < kHumanoidFootTolerance &&
         std::fabs(box.max.x) < kHumanoidMaxWidth &&
         std::fabs(box.max.y) < kHumanoidMaxWidth;
}

// The entity's own copy is authoritative only for something with no physics
// component, and it agreed with the truth in all 33 characters measured -- so
// the component is the fallback and not the first choice.
[[nodiscard]] std::optional<math::Matrix4> ReadWorldMatrix(
    std::uintptr_t address) noexcept {
  const std::optional<math::Matrix4> kOwn{
      Engine{address}.Field<math::Matrix4>(kEntityTransform)};
  if (kOwn && kOwn->IsRigid(kUnitTolerance, kAxisTolerance)) {
    return kOwn;
  }

  const std::optional<math::Matrix4> kFromComponent{
      ReadComponentWorldMatrix(address)};
  if (kFromComponent) {
    return kFromComponent;
  }

  return kOwn;
}

}  // namespace

std::optional<RawEntity> ReadEntity(std::uintptr_t address) noexcept {
  const Engine kEntity{address};
  const std::optional<EnginePtr> kVtable{kEntity.Field<EnginePtr>(0)};
  const std::optional<math::Matrix4> kTransform{ReadWorldMatrix(address)};
  const std::optional<math::Aabb> kBox{kEntity.Field<math::Aabb>(kEntityAabb)};
  if (!kVtable || !kTransform || !kBox) {
    return std::nullopt;
  }

  return RawEntity{.address = address,
                   .vtable = static_cast<std::uintptr_t>(*kVtable),
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

  const math::Vec3 kPosition{raw.transform.Translation()};
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

}  // namespace afw::games::fc3
