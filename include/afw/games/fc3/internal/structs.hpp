#pragma once

#include <afw/math/vec3.hpp>
#include <cstdint>

namespace afw::games::fc3 {

// The engine stores three consecutive floats wherever a point or an axis goes,
// which is exactly Vec3's layout, so these are read straight out of its memory
// with no intermediate type.
using math::Vec3;

struct CameraBasis {
  Vec3 position{};
  Vec3 forward{};
  Vec3 upAxis{};
  Vec3 right{};
  float fovRadians{};
  float fovHorizontalRadians{};
};

struct Viewport {
  float width{};
  float height{};
};

struct EntityInfo {
  std::uintptr_t address{};
  std::uintptr_t vtable{};
  Vec3 position{};
  // The object's own axes in world space; the box at +0x6C is in that space.
  Vec3 axisX{};
  Vec3 axisY{};
  Vec3 axisZ{};
  Vec3 aabbMin{};
  Vec3 aabbMax{};
  float height{};
  std::int32_t army{};
  std::int32_t faction{};
  std::uint8_t ownArmyBit{};
  std::uint8_t hostileMask{};
  std::uint8_t reverseHostileMask{};
  std::uint8_t deadByte{};
  bool pawn{};
  bool humanoid{};
  bool dead{};
};

}  // namespace afw::games::fc3
