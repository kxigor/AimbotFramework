#pragma once

#include <afw/aimbot/aimbot.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/math/vec3.hpp>
#include <cstdint>
#include <optional>

namespace afw::dll {

// The shim's own work, split by what each part is for. Everything here is
// private to aimbot.dll: it is the glue between the engine-agnostic libraries
// and this one title, and nothing outside links it.

// Half the cone the user set, in radians. The setting is a full angle in
// degrees because that is what a field of view reads as; every use of it is a
// half-angle from the view axis.
[[nodiscard]] float HalfConeRadians(float degrees) noexcept;

// How much the assist wants this entity, or nothing when it does not want it at
// all. The dead are never wanted, whatever their kind.
[[nodiscard]] std::optional<float> PriorityOf(
    const games::fc3::EntityInfo& entity,
    const games::fc3::PawnInfo* self) noexcept;

[[nodiscard]] std::optional<aimbot::Target> Pick(
    const games::fc3::EntitySnapshot& snapshot,
    const games::fc3::CameraBasis& camera) noexcept;

// The camera state object the view angles are written through, found once and
// then trusted for as long as it keeps reproducing the camera's own forward
// vector.
[[nodiscard]] std::uintptr_t CameraState(
    const games::fc3::EntitySnapshot& snapshot,
    const math::Vec3& forward) noexcept;

// Files a trace against everything the assist would consider, so that by the
// time a target is picked the answer to "can the bullet get there" is already
// in hand. Runs on the entity update rather than on the frame: it reads the
// world.
void UpdateReach() noexcept;

void RunAssist() noexcept;

// Applies or removes the weapon patches to match the settings.
void SteadyAim() noexcept;

void ProbeTrace();

void LogStatus(std::uint64_t frames);

}  // namespace afw::dll
