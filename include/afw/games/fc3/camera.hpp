#pragma once

#include <afw/games/fc3/internal/structs.hpp>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

[[nodiscard]] std::expected<CameraBasis, std::error_code> ReadCamera() noexcept;

[[nodiscard]] math::ViewAngles ViewAnglesTo(
    const math::Vec3& direction) noexcept;

[[nodiscard]] std::optional<std::uintptr_t> FindViewAngles(
    std::uintptr_t playerEntity, const math::Vec3& forward) noexcept;

[[nodiscard]] bool ReproducesTheView(std::uintptr_t state,
                                     const math::Vec3& forward) noexcept;

[[nodiscard]] std::optional<math::ViewAngles> ReadViewAngles(
    std::uintptr_t state) noexcept;

bool WriteViewAngles(std::uintptr_t state,
                     const math::ViewAngles& angles) noexcept;

}  // namespace afw::games::fc3
