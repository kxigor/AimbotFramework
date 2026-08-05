#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

// Files a walk of the collision world and hands back the id it was filed
// under. The answer is not ready in the same breath, which is why the id comes
// back rather than the result.
[[nodiscard]] std::optional<int> TraceSegment(
    const math::Vec3& from, const math::Vec3& target) noexcept;

struct TraceHit {
  math::Vec3 point{};
};

struct TraceAnswer {
  std::uint32_t hits{};
  TraceHit first{};
};

// Nothing when the walk has not been filed yet, and an empty answer when it was
// filed and met nothing.
[[nodiscard]] std::optional<TraceAnswer> TraceLook(int query) noexcept;

// Keeps one standing answer per entity, refreshed on a slow cycle: a walk costs
// a query and the answer changes at the speed a body moves, not a frame.
void ObserveReach(std::uintptr_t entity, const math::Vec3& eye,
                  const math::Vec3& point, std::uint64_t frame) noexcept;

[[nodiscard]] float ReachOf(std::uintptr_t entity) noexcept;

}  // namespace afw::games::fc3
