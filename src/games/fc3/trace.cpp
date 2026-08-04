#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <afw/process/memory.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] std::optional<std::uintptr_t> QueryCore() noexcept {
  const std::expected<std::uintptr_t, std::error_code> kWorld{
      process::ReadPointerAs<EnginePtr>(Resolve(kCollisionQueryWorld))};
  if (!kWorld || *kWorld == 0) {
    return std::nullopt;
  }

  const std::expected<std::uintptr_t, std::error_code> kCore{
      process::ReadPointerAs<EnginePtr>(*kWorld + kQueryCore)};
  if (!kCore || *kCore == 0) {
    return std::nullopt;
  }

  return *kCore;
}

// One table of query records, searched for the one the walk was filed under.
// What comes back is where that record keeps its hits.
[[nodiscard]] std::optional<std::uintptr_t> FindRecord(std::uintptr_t core,
                                                       std::uintptr_t table,
                                                       std::uintptr_t counter,
                                                       int query) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kBase{
      process::ReadPointerAs<EnginePtr>(core + table)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(core + counter)};
  if (!kBase || !kCount || *kBase == 0 || *kCount > kMaxQueryRecords) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::uintptr_t kRecord{*kBase + (index * kQueryRecordStride)};
    const std::expected<std::int32_t, std::error_code> kFiled{
        process::ReadAt<std::int32_t>(kRecord)};
    if (kFiled && *kFiled == query) {
      return kRecord + kQueryRecordHits;
    }
  }

  return std::nullopt;
}

}  // namespace

#ifdef __i386__

namespace {

// The engine's own convention here is __thiscall: the world in ECX, twelve
// arguments on the stack, and RET 0x30 to clear them. GCC will not put that
// attribute on a plain function pointer, so this is spelled as fastcall
// instead, whose first two arguments land in ECX and EDX and whose rest go on
// the stack in order -- identical once the EDX slot is written off as a
// scratch the callee never reads.
//
// All twelve stack arguments are spelled out rather than defaulted: the callee
// clears exactly twelve on its way out, so passing one fewer would leave the
// stack short by a slot. Their order is the forwarder's own, which drops the
// first and passes the eleven after it straight through.
using SegmentQueryFn = int(__attribute__((fastcall)) *)(
    void* world, int scratch, int dropped, int mode, const float* from,
    const float* delta, const void* filter, int zeroA, int four, int zeroB,
    int zeroC, int one, int walkNow, int zeroD);

constexpr int kMode{1};
constexpr int kFour{4};
constexpr int kOne{1};

// The game itself always asks for the deferred walk, and going off what the
// game does is the whole reason these arguments are right at all.
constexpr int kWalkNow{0};

// An object the walk reads two words out of and folds into the mask it tests
// with. Only those two matter, but the head is filled in as the game fills it
// so that anything else reaching for the object finds what it expects.
struct Filter {
  std::uint32_t vtable{};
  std::uint32_t mask{};
  std::uint32_t layer{};
};

}  // namespace

std::optional<int> TraceSegment(const Vec3& from, const Vec3& target) noexcept {
  if (!from.IsFinite() || !target.IsFinite()) {
    return std::nullopt;
  }

  const std::expected<std::uintptr_t, std::error_code> kWorld{
      process::ReadPointerAs<EnginePtr>(Resolve(kCollisionQueryWorld))};
  if (!kWorld || *kWorld == 0) {
    return std::nullopt;
  }

  const std::array<float, 3> kFrom{from.x, from.y, from.z};
  const std::array<float, 3> kDelta{target.x - from.x, target.y - from.y,
                                    target.z - from.z};
  const Filter kFilter{
      .vtable = static_cast<std::uint32_t>(Resolve(kQueryFilterVtable)),
      .mask = kQueryFilterMask,
      .layer = 0};
  const SegmentQueryFn kQuery{
      std::bit_cast<SegmentQueryFn>(Resolve(kSegmentQuery))};

  return kQuery(std::bit_cast<void*>(*kWorld), 0, 0, kMode, kFrom.data(),
                kDelta.data(), &kFilter, 0, kFour, 0, 0, kOne, kWalkNow, 0);
}

#else

std::optional<int> TraceSegment(const Vec3& /*from*/,
                                const Vec3& /*target*/) noexcept {
  return std::nullopt;
}

#endif

namespace {

[[nodiscard]] std::optional<TraceHit> ReadHit(std::uintptr_t hit) noexcept {
  const std::expected<Vec3, std::error_code> kPoint{process::ReadAt<Vec3>(hit)};
  if (!kPoint || !kPoint->IsFinite()) {
    return std::nullopt;
  }

  return TraceHit{.point = *kPoint};
}

}  // namespace

std::optional<TraceAnswer> TraceLook(int query) noexcept {
  const std::optional<std::uintptr_t> kCore{QueryCore()};
  if (!kCore) {
    return std::nullopt;
  }

  std::optional<std::uintptr_t> record{
      FindRecord(*kCore, kQueryTableA, kQueryCountA, query)};
  if (!record) {
    record = FindRecord(*kCore, kQueryTableB, kQueryCountB, query);
  }
  if (!record) {
    return std::nullopt;
  }

  const std::expected<std::uintptr_t, std::error_code> kBase{
      process::ReadPointerAs<EnginePtr>(*record)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(*record + sizeof(EnginePtr))};
  if (!kBase || !kCount || *kCount > kMaxHits) {
    return std::nullopt;
  }
  if (*kCount == 0 || *kBase == 0) {
    return TraceAnswer{};
  }

  const std::optional<TraceHit> kFirst{ReadHit(*kBase)};
  if (!kFirst) {
    return std::nullopt;
  }

  return TraceAnswer{.hits = *kCount, .first = *kFirst};
}

namespace {

constexpr std::size_t kReachRows{32};
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
  static std::size_t next{0};

  for (Reach& row : Reaches()) {
    if (row.entity == entity) {
      return row;
    }
  }

  Reach& taken{Reaches().at(next)};
  next = (next + 1) % kReachRows;
  taken = Reach{.entity = entity};

  return taken;
}

// A walk that met nothing got there. So did one whose first hit is the target
// itself, which is anything landing within kReachSlack of the far end -- the
// aim point sits inside a body, so the body is struck just short of it.
[[nodiscard]] bool Reached(const TraceAnswer& answer, const Vec3& eye,
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

void ObserveReach(std::uintptr_t entity, const Vec3& eye, const Vec3& point,
                  std::uint64_t frame) noexcept {
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
