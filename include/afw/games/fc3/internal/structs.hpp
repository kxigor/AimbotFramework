#pragma once

#include <afw/math/aabb.hpp>
#include <afw/math/matrix4.hpp>
#include <afw/math/vec3.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

// The engine stores points, axes and boxes as runs of consecutive floats, which
// is what these types already are, so they are read straight out of its memory
// with no intermediate representation.
using math::Aabb;
using math::Matrix4;
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

// Present only on an entity that passed the army invariant, so every field here
// is meaningful whenever the record itself is.
struct PawnInfo {
  std::int32_t army{};
  std::int32_t faction{};
  std::uint8_t ownArmyBit{};
  std::uint8_t hostileMask{};
  std::uint8_t reverseHostileMask{};
  std::uint8_t deadByte{};
  bool dead{};
};

struct EntityInfo {
  std::uintptr_t address{};
  std::uintptr_t vtable{};
  // Rows 0..2 are the object's axes in world space, row 3 its position. The box
  // is in that same object space.
  Matrix4 transform{};
  Aabb box{};
  std::optional<PawnInfo> pawn;
  bool humanoid{};
};

}  // namespace afw::games::fc3
