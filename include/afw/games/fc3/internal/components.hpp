#pragma once

#include <afw/games/fc3/internal/read.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

// An entity's component array, with its count already bounded. Three walks
// need it -- the agent, the world matrix and the camera state -- and each of
// them used to re-read and re-check the pair for itself.
[[nodiscard]] std::optional<EngineArray> Components(
    std::uintptr_t entity) noexcept;

// The component the engine reaches a moving entity's world matrix through. It
// picks by a class hash obtained through a virtual call, which a read-only walk
// cannot make; this asks the same question of the data instead, because only
// the right component holds an actually rigid basis at that offset.
struct RigidComponent {
  std::uintptr_t address{};
  math::Matrix4 transform{};
};

// `posed` narrows the search to a component that also carries a skeleton, which
// is what a bone lookup needs and a bare transform does not.
[[nodiscard]] std::optional<RigidComponent> FindRigidComponent(
    std::uintptr_t address, bool posed) noexcept;

}  // namespace afw::games::fc3
