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

// All three of those walks are the same shape: over the components, skipping
// the empty slots, take the first that answers. Only the question differs, so
// only the question is passed in. `ask` is called with each non-empty component
// and returns an empty optional to mean "not this one"; the first engaged
// answer is the result.
template <typename Ask>
[[nodiscard]] auto FirstComponent(std::uintptr_t entity, Ask ask) noexcept
    -> decltype(ask(std::uintptr_t{})) {
  const std::optional<EngineArray> kParts{Components(entity)};
  if (!kParts) {
    return {};
  }

  for (std::uint32_t index{0}; index < kParts->Size(); ++index) {
    const std::optional<std::uintptr_t> kPart{kParts->Slot(index)};
    if (!kPart) {
      continue;
    }

    if (auto answer{ask(*kPart)}) {
      return answer;
    }
  }

  return {};
}

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

// A fallback, not the first choice. Several components of one entity carry a
// rigid basis and they do not agree: measured across 33 characters, taking the
// first rigid one put a single pirate 118 m from where he stood. The entity's
// own matrix agreed with the truth in all 33 cases.
[[nodiscard]] std::optional<math::Matrix4> ReadComponentWorldMatrix(
    std::uintptr_t address) noexcept;

// Walks a component back to its entity. What comes back is confirmed to be an
// entity before it is handed out.
[[nodiscard]] std::optional<std::uintptr_t> OwnerOfComponent(
    std::uintptr_t address) noexcept;

}  // namespace afw::games::fc3
