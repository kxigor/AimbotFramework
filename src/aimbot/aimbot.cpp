#include <afw/aimbot/aimbot.hpp>

namespace afw::aimbot {

math::Vec3 AimPoint(const games::fc3::EntityInfo& entity) noexcept {
  if (entity.head) {
    return *entity.head;
  }
  if (entity.body) {
    return *entity.body;
  }

  return entity.transform.TransformPoint(entity.box.Centre());
}

}  // namespace afw::aimbot
