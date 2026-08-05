#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

[[nodiscard]] std::expected<CameraBasis, std::error_code> ReadCamera() noexcept;

// A direction turned into the angles the game itself stores. Its yaw is
// measured a quarter turn off the world's, so this is not just an arctangent.
[[nodiscard]] math::ViewAngles ViewAnglesTo(
    const math::Vec3& direction) noexcept;

// The engine keeps no class for the camera state, so it is identified by
// agreeing with a fact read independently of it: nothing else reproduces the
// camera's own forward vector by accident.
[[nodiscard]] std::optional<std::uintptr_t> FindViewAngles(
    std::uintptr_t playerEntity, const math::Vec3& forward) noexcept;

[[nodiscard]] bool ReproducesTheView(std::uintptr_t state,
                                     const math::Vec3& forward) noexcept;

[[nodiscard]] std::optional<math::ViewAngles> ReadViewAngles(
    std::uintptr_t state) noexcept;

// Writes all three copies the engine keeps. Writing fewer leaves the new
// heading to be overwritten before the frame is drawn.
bool WriteViewAngles(std::uintptr_t state,
                     const math::ViewAngles& angles) noexcept;

}  // namespace afw::games::fc3
