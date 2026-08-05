#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

// The skeleton hanging off a component: where its entries start and how many
// there are, bounded. Two walks want it and they used to state the pair of
// offsets and the bound separately -- the one that picks the component by
// whether it carries a pose, and the one that then reads a bone out of it.
struct BoneTable {
  std::uintptr_t base{};
  std::uint32_t count{};
};

[[nodiscard]] std::optional<BoneTable> BonesOf(
    std::uintptr_t component) noexcept;

// Where the skeleton's "Head" and "Spine" bones stand, in world space. Either
// is absent when this skeleton has no such bone. Both come out of one walk
// because finding the component that carries the pose is the expensive half and
// the second bone is then only another pass over an array already in hand.
struct BonePoints {
  std::optional<math::Vec3> head;
  std::optional<math::Vec3> body;
};

// Costs a walk of the entity's components, so it is asked only of what the
// engine drives with a pawn agent rather than of every entity in the world.
[[nodiscard]] BonePoints ReadBonePoints(std::uintptr_t address) noexcept;

}  // namespace afw::games::fc3
