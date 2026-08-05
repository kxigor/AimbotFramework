#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/read.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <optional>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] std::optional<std::uintptr_t> QueryCore() noexcept {
  const std::optional<std::uintptr_t> kWorld{
      Engine{Resolve(kCollisionQueryWorld)}.Pointer(0)};
  if (!kWorld) {
    return std::nullopt;
  }

  return Engine{*kWorld}.Pointer(kQueryCore);
}

[[nodiscard]] std::optional<std::uintptr_t> FindRecord(std::uintptr_t core,
                                                       std::uintptr_t table,
                                                       std::uintptr_t counter,
                                                       int query) noexcept {
  const Engine kCore{core};
  const std::optional<std::uintptr_t> kBase{kCore.Pointer(table)};
  const std::optional<std::uint32_t> kCount{
      kCore.CountAtMost(counter, kMaxQueryRecords)};
  if (!kBase || !kCount) {
    return std::nullopt;
  }

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    const std::uintptr_t kRecord{*kBase + (index * kQueryRecordStride)};
    const std::optional<std::int32_t> kFiled{
        Engine{kRecord}.Field<std::int32_t>(0)};
    if (kFiled && *kFiled == query) {
      return kRecord + kQueryRecordHits;
    }
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<TraceHit> ReadHit(std::uintptr_t hit) noexcept {
  const std::optional<math::Vec3> kPoint{Engine{hit}.Field<math::Vec3>(0)};
  if (!kPoint || !kPoint->IsFinite()) {
    return std::nullopt;
  }

  return TraceHit{.point = *kPoint};
}

}  // namespace

#ifdef __i386__

namespace {

using SegmentQueryFn = int(__attribute__((fastcall)) *)(
    void* world, int scratch, int dropped, int mode, const float* from,
    const float* delta, const void* filter, int zeroA, int four, int zeroB,
    int zeroC, int one, int walkNow, int zeroD);

constexpr int kMode{1};
constexpr int kFour{4};
constexpr int kOne{1};

constexpr int kWalkNow{0};

struct Filter {
  std::uint32_t vtable{};
  std::uint32_t mask{};
  std::uint32_t layer{};
};

}  // namespace

std::optional<int> TraceSegment(const math::Vec3& from,
                                const math::Vec3& target) noexcept {
  if (!from.IsFinite() || !target.IsFinite()) {
    return std::nullopt;
  }

  const std::optional<std::uintptr_t> kWorld{
      Engine{Resolve(kCollisionQueryWorld)}.Pointer(0)};
  if (!kWorld) {
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

std::optional<int> TraceSegment(const math::Vec3& /*from*/,
                                const math::Vec3& /*target*/) noexcept {
  return std::nullopt;
}

#endif

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

  const Engine kHits{*record};
  const std::optional<EnginePtr> kBase{kHits.Field<EnginePtr>(0)};
  const std::optional<std::uint32_t> kCount{
      kHits.CountAtMost(sizeof(EnginePtr), kMaxHits)};
  if (!kBase || !kCount) {
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

}  // namespace afw::games::fc3
