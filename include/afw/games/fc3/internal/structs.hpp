#pragma once

#include <afw/math/aabb.hpp>
#include <afw/math/angle.hpp>
#include <afw/math/matrix4.hpp>
#include <afw/math/vec3.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

// The engine stores points, axes and boxes as runs of consecutive floats, which
// is what these types already are, so they are read straight out of its memory
// with no intermediate representation.

struct CameraBasis {
  math::Vec3 position{};
  math::Vec3 forward{};
  math::Vec3 upAxis{};
  math::Vec3 right{};
  float fovRadians{};
  float fovHorizontalRadians{};
};

// One value per reason, because "it did not appear" is the symptom that costs
// the most time here: an NPC standing in front of you and not drawn has to be
// traceable to the exact test that dropped it.
//
// Count is a sentinel and never a reason. The census sizes an array by the
// count and indexes it by the enumerator itself, inside a noexcept walk on the
// engine's thread, so a reason appended without the count following it would be
// a terminate in the game rather than a wrong number. Deriving the count keeps
// the two in step; a hand-written number only catches an insertion in the
// middle, which is not how a reason gets added.
enum class Rejection : std::uint8_t {
  NoVtable,
  NotRigid,
  BadBox,
  NonFinite,
  BeyondTheWorld,
  TemplateSpawn,
  Count,
};

constexpr std::size_t kRejectionCount{
    static_cast<std::size_t>(Rejection::Count)};

// Read from a live civilian: army -1 with every bit set in the own-army mask,
// faction -2. Soldiers instead carry an army in range and exactly the one bit
// that names it.
constexpr std::int32_t kNoArmy{-1};

// enumPawnAgentType, registered next to the field it describes by
// CBasePawnEntity_RegisterProperties at 0x10bdcac2. The engine saying what a
// character is, so none of it has to be guessed from the shape of a box.
constexpr std::uint32_t kAgentUnknown{0};
constexpr std::uint32_t kAgentPlayer{1};
constexpr std::uint32_t kAgentCompanion{2};
constexpr std::uint32_t kAgentCivilian{3};

// enumFaction, from the same registrar. Worth more than the army masks for
// deciding sides: the masks are filled by a post-load callback a spawned pawn
// never runs, so the player's own are zero and every mask comparison against
// them answers "not hostile".
constexpr std::int32_t kFactionUnknown{0};
constexpr std::int32_t kFactionRebel{1};
constexpr std::int32_t kFactionPirate{2};
constexpr std::int32_t kFactionPrivateer{3};

// Only meaningful on a CBasePawnEntity, which ReadPawn confirms with the
// registry first. army is kNoArmy for anything the engine does not enlist.
// Which agent drives the entity. Siblings in the registry, and the engine's own
// discriminator: CastToCPawnAgent fails on an animal and CastToCAnimalAgentFC3
// fails on a human.
// No Count sentinel here, unlike Rejection: ClassDefOf switches on this without
// a usable default, so -Werror=switch-enum already refuses a kind that has been
// added and not given a class definition. A sentinel would only make that
// switch name a case that is not a kind.
enum class AgentKind : std::uint8_t { Pawn, Animal, SimpleAnimal, Vehicle };

constexpr std::size_t kAgentKindCount{4};

static_assert(kAgentKindCount ==
              static_cast<std::size_t>(AgentKind::Vehicle) + 1);

struct PawnInfo {
  AgentKind kind{AgentKind::Pawn};
  std::int32_t army{kNoArmy};
  std::int32_t faction{kFactionUnknown};
  std::uint32_t agentType{};
  std::uint8_t ownArmyBit{};
  std::uint8_t hostileMask{};
  std::uint8_t reverseHostileMask{};
  bool dead{};
};

struct EntityInfo {
  std::uintptr_t address{};
  std::uintptr_t vtable{};
  // Rows 0..2 are the object's axes in world space, row 3 its position. The box
  // is in that same object space.
  math::Matrix4 transform{};
  math::Aabb box{};
  std::optional<PawnInfo> pawn;
  // Where the skeleton's "Head" bone stands this frame, in world space. Only
  // read for entities the engine drives with a pawn agent, and absent when that
  // skeleton has no such bone.
  std::optional<math::Vec3> head;
  // The "Spine" bone, in the same space and read under the same conditions. It
  // is what an animal is aimed at: nothing else on one is both reliably present
  // and reliably inside the body.
  std::optional<math::Vec3> body;
  bool humanoid{};
};

}  // namespace afw::games::fc3
