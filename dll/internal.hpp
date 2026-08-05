#pragma once

#include <afw/aimbot/aimbot.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/math/vec3.hpp>
#include <cstdint>
#include <optional>

namespace afw::dll {

[[nodiscard]] float HalfConeRadians(float degrees) noexcept;

[[nodiscard]] std::optional<float> PriorityOf(
    const games::fc3::EntityInfo& entity,
    const games::fc3::PawnInfo* self) noexcept;

[[nodiscard]] std::optional<aimbot::Target> Pick(
    const games::fc3::EntitySnapshot& snapshot,
    const games::fc3::CameraBasis& camera) noexcept;

[[nodiscard]] std::uintptr_t CameraState(
    const games::fc3::EntitySnapshot& snapshot,
    const math::Vec3& forward) noexcept;

void UpdateReach() noexcept;

void RunAssist() noexcept;

void SteadyAim() noexcept;

void ProbeTrace();

void LogStatus(std::uint64_t frames);

}  // namespace afw::dll
