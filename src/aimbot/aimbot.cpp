#include <afw/aimbot/aimbot.hpp>
#include <afw/math/angle.hpp>
#include <algorithm>
#include <cmath>

namespace afw::aimbot {

ViewAngles AnglesOf(const Vec3& direction) noexcept {
  const Vec3 kUnit{direction.Normalised()};
  const float kGround{std::hypot(kUnit.x, kUnit.y)};
  if (kGround <= 0.0F) {
    return ViewAngles{
        .yaw = 0.0F,
        .pitch = kUnit.z >= 0.0F ? math::kQuarterTurn : -math::kQuarterTurn};
  }

  return ViewAngles{.yaw = std::atan2(kUnit.y, kUnit.x),
                    .pitch = std::atan2(kUnit.z, kGround)};
}

ViewAngles AnglesTo(const Vec3& from, const Vec3& target) noexcept {
  return AnglesOf(target - from);
}

ViewAngles Difference(const ViewAngles& from,
                      const ViewAngles& target) noexcept {
  return ViewAngles{.yaw = math::WrapAngle(target.yaw - from.yaw),
                    .pitch = math::WrapAngle(target.pitch - from.pitch)};
}

ViewAngles Share(const ViewAngles& correction, float share) noexcept {
  const float kClamped{std::clamp(share, 0.0F, 1.0F)};

  return ViewAngles{.yaw = correction.yaw * kClamped,
                    .pitch = correction.pitch * kClamped};
}

float AngleBetween(const Vec3& lhs, const Vec3& rhs) noexcept {
  const Vec3 kLeft{lhs.Normalised()};
  const Vec3 kRight{rhs.Normalised()};
  if (kLeft.Length() <= 0.0F || kRight.Length() <= 0.0F) {
    return math::kHalfTurn;
  }

  return std::acos(std::clamp(kLeft.Dot(kRight), -1.0F, 1.0F));
}

Vec3 Lead(const Vec3& point, const Vec3& velocity, float seconds) noexcept {
  if (!std::isfinite(seconds) || seconds <= 0.0F || !velocity.IsFinite()) {
    return point;
  }

  return point + (velocity * seconds);
}

float TravelTime(float distance, float speed) noexcept {
  if (!std::isfinite(distance) || !std::isfinite(speed) || speed <= 0.0F ||
      distance <= 0.0F) {
    return 0.0F;
  }

  return distance / speed;
}

void MotionTable::Observe(std::uintptr_t entity, const Vec3& point,
                          float seconds) noexcept {
  if (entity == 0 || !point.IsFinite()) {
    return;
  }

  for (Motion& row : rows_) {
    if (row.entity != entity) {
      continue;
    }
    if (seconds > 0.0F) {
      const Vec3 kStep{(point - row.point) * (1.0F / seconds)};
      const bool kBelievable{kStep.Length() <= kMaxTargetSpeed};
      row.velocity = kBelievable ? kStep : Vec3{};
      row.settled = kBelievable;
    }
    row.point = point;
    return;
  }

  rows_.at(next_) = Motion{.entity = entity, .point = point};
  next_ = (next_ + 1) % kRows;
}

Vec3 MotionTable::VelocityOf(std::uintptr_t entity) const noexcept {
  for (const Motion& kRow : rows_) {
    if (kRow.entity == entity && kRow.settled) {
      return kRow.velocity;
    }
  }

  return Vec3{};
}

void MotionTable::Forget() noexcept {
  rows_.fill(Motion{});
  next_ = 0;
}

Vec3 AimPoint(const games::fc3::EntityInfo& entity) noexcept {
  if (entity.head) {
    return *entity.head;
  }
  if (entity.body) {
    return *entity.body;
  }

  return entity.transform.TransformPoint(entity.box.Centre());
}

}  // namespace afw::aimbot
