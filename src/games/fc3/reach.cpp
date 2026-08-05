#include <afw/games/fc3/internal/structs.hpp>
#include <afw/games/fc3/trace.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

namespace {

constexpr std::size_t kReachRows{64};
constexpr std::uint64_t kReachInterval{15};
constexpr std::uint64_t kReachGiveUp{45};
constexpr float kReachSlack{1.5F};

struct Reach {
  std::uintptr_t entity{};
  std::optional<int> pending;
  float span{};
  float value{1.0F};
  std::uint64_t asked{};
};

[[nodiscard]] std::array<Reach, kReachRows>& Reaches() noexcept {
  static std::array<Reach, kReachRows> rows{};

  return rows;
}

[[nodiscard]] Reach& RowFor(std::uintptr_t entity) noexcept {
  Reach* oldest{&Reaches().front()};
  for (Reach& row : Reaches()) {
    if (row.entity == entity) {
      return row;
    }
    if (row.asked < oldest->asked) {
      oldest = &row;
    }
  }

  *oldest = Reach{.entity = entity};

  return *oldest;
}

[[nodiscard]] bool Reached(const TraceAnswer& answer, const math::Vec3& eye,
                           float span) noexcept {
  if (answer.hits == 0) {
    return true;
  }

  return (answer.first.point - eye).Length() + kReachSlack >= span;
}

}  // namespace

float ReachOf(std::uintptr_t entity) noexcept {
  for (const Reach& kRow : Reaches()) {
    if (kRow.entity == entity) {
      return kRow.value;
    }
  }

  return 1.0F;
}

void ObserveReach(std::uintptr_t entity, const math::Vec3& eye,
                  const math::Vec3& point, std::uint64_t frame) noexcept {
  Reach& row{RowFor(entity)};
  if (row.pending) {
    const std::optional<TraceAnswer> kAnswer{TraceLook(*row.pending)};
    if (kAnswer) {
      row.value = Reached(*kAnswer, eye, row.span) ? 1.0F : 0.0F;
      row.pending.reset();
    } else if (frame - row.asked > kReachGiveUp) {
      row.pending.reset();
    }
    return;
  }
  if (frame - row.asked < kReachInterval) {
    return;
  }

  row.span = (point - eye).Length();
  row.asked = frame;
  row.pending = TraceSegment(eye, point);
}

}  // namespace afw::games::fc3
