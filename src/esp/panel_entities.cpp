#include <imgui.h>

#include <afw/esp/internal/palette.hpp>
#include <afw/esp/internal/panel.hpp>
#include <afw/esp/internal/projection.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/kind.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

namespace afw::esp {

namespace {

constexpr float kAimTolerance{0.95F};
constexpr std::size_t kNearbyRows{8};
constexpr float kNearbyRange{25.0F};
constexpr float kMinAimDistance{1.0F};

constexpr std::int32_t kNoPawnField{-99};

struct Nearby {
  const games::fc3::EntityInfo* entity{nullptr};
  float distance{0.0F};
};

using NearbyRows = std::array<Nearby, kNearbyRows>;

[[nodiscard]] Nearby UnderCrosshair(const math::Vec3& eye,
                                    const math::Vec3& forward) noexcept {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};

  Nearby hit{};
  Nearby spare{};
  float best{kAimTolerance};
  float fallback{kAimTolerance};

  for (const games::fc3::EntityInfo& kEntity :
       games::fc3::Entities(kSnapshot)) {
    const math::Vec3 kDelta{kEntity.transform.Translation() - eye};
    const float kDistance{kDelta.Length()};
    if (kDistance < kMinAimDistance || kDistance > kDrawDistance) {
      continue;
    }

    const float kDot{kDelta.Normalised().Dot(forward)};
    if (kEntity.pawn) {
      if (kDot > best) {
        best = kDot;
        hit = Nearby{.entity = &kEntity, .distance = kDistance};
      }
      continue;
    }
    if (hit.entity == nullptr && kDot > fallback) {
      fallback = kDot;
      spare = Nearby{.entity = &kEntity, .distance = kDistance};
    }
  }

  return hit.entity != nullptr ? hit : spare;
}

void Insert(NearbyRows& rows, std::size_t& used, const Nearby& row) noexcept {
  std::size_t slot{used < kNearbyRows ? used : kNearbyRows - 1};
  if (used < kNearbyRows) {
    ++used;
  } else if (rows.at(slot).distance <= row.distance) {
    return;
  }

  while (slot > 0 && rows.at(slot - 1).distance > row.distance) {
    rows.at(slot) = rows.at(slot - 1);
    --slot;
  }
  rows.at(slot) = row;
}

void ShowNearby(const math::Vec3& eye) {
  const games::fc3::EntitySnapshot& kSnapshot{games::fc3::LastSnapshot()};
  const games::fc3::PawnInfo* const kSelf{games::fc3::FindSelf(kSnapshot)};
  NearbyRows rows{};
  std::size_t used{0};

  for (const games::fc3::EntityInfo& kEntity :
       games::fc3::Entities(kSnapshot)) {
    const float kDistance{(kEntity.transform.Translation() - eye).Length()};
    if (kDistance <= kNearbyRange) {
      Insert(rows, used, Nearby{.entity = &kEntity, .distance = kDistance});
    }
  }

  ImGui::SeparatorText("nearest");
  for (std::size_t index{0}; index < used; ++index) {
    const games::fc3::EntityInfo& kEntity{*rows.at(index).entity};
    ImGui::Text("%8llX %5.1fm %-8s a%-3d f%-3d",
                static_cast<std::uint64_t>(kEntity.address),
                static_cast<double>(rows.at(index).distance),
                games::fc3::KindName(games::fc3::KindOf(kEntity, kSelf)),
                kEntity.pawn ? kEntity.pawn->army : kNoPawnField,
                kEntity.pawn ? kEntity.pawn->faction : kNoPawnField);
  }
}

void ShowInspector(const math::Vec3& eye, const math::Vec3& forward) {
  const Nearby kHit{UnderCrosshair(eye, forward)};
  const games::fc3::EntityInfo* const kNearest{kHit.entity};

  ImGui::SeparatorText("crosshair");
  if (kNearest == nullptr) {
    ImGui::TextUnformatted("nothing within the aim cone");
    return;
  }

  ImGui::Text("%-14s %llX", "entity",
              static_cast<std::uint64_t>(kNearest->address));
  ImGui::Text("%-14s %llX", "vtable",
              static_cast<std::uint64_t>(kNearest->vtable));
  ImGui::Text("%-14s %.1f m", "distance", static_cast<double>(kHit.distance));
  ShowVector("position", kNearest->transform.Translation());
  ShowVector("aabb min", kNearest->box.min);
  ShowVector("aabb max", kNearest->box.max);
  if (!kNearest->pawn) {
    ImGui::TextUnformatted("not a pawn: no army fields");
    return;
  }

  const games::fc3::PawnInfo& kPawn{*kNearest->pawn};
  ImGui::Text("%-14s %d  faction %d", "army", kPawn.army, kPawn.faction);
  ImGui::Text("%-14s %u", "agent type", kPawn.agentType);
  ImGui::Text("%-14s own %02X  hostile %02X  reverse %02X", "masks",
              kPawn.ownArmyBit, kPawn.hostileMask, kPawn.reverseHostileMask);
}

}  // namespace

void ShowEntities(const math::Vec3& eye, const math::Vec3& forward) {
  ShowInspector(eye, forward);
  ShowNearby(eye);
}

}  // namespace afw::esp
