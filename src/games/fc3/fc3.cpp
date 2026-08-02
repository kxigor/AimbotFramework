#include <windows.h>

#include <afw/games/fc3/error.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/process/memory.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

namespace {

constexpr PCWSTR kModuleName{L"FC3_d3d11.dll"};
constexpr std::uintptr_t kGhidraImageBase{0x10000000};

constexpr std::uintptr_t kCameraManagerContainer{0x11e137b4};
constexpr std::uintptr_t kViewportBlock{0x11e5a654};

constexpr std::uintptr_t kContainerManagerSlot{0x08};
constexpr std::uintptr_t kContainerLiveFlag{0x0C};
constexpr std::uintptr_t kManagerViewSlot{0x04};
constexpr std::uintptr_t kViewOffset{0x18};

constexpr std::uintptr_t kViewPosition{0x04};
// The frame is drawn from the horizontal angle; the vertical field is
// configured separately and does not follow it.
constexpr std::uintptr_t kViewHorizontalFov{0x24};
constexpr std::uintptr_t kViewVerticalFov{0x2C};
constexpr std::uintptr_t kViewForward{0x64};
constexpr std::uintptr_t kViewUp{0x70};
constexpr std::uintptr_t kViewRight{0x7C};

constexpr std::uintptr_t kViewportWidth{0x10};
constexpr std::uintptr_t kViewportHeight{0x14};

constexpr std::uintptr_t kRunAllGroupSlotAddress{0x115a93c0};

constexpr std::uintptr_t kSchedulerGroups{0x20};
constexpr std::uintptr_t kSchedulerGroupCount{0x24};
constexpr std::uintptr_t kGroupEntityGroup{0x08};
constexpr std::uintptr_t kEntityGroupArray{0x04};
constexpr std::uintptr_t kEntityGroupCount{0x08};

constexpr std::uint32_t kMaxGroups{256};
constexpr std::uint32_t kMaxGroupEntities{4096};

// Rows of the row-major world matrix at +0x20; row 3 is the translation.
constexpr std::uintptr_t kEntityAxisX{0x20};
constexpr std::uintptr_t kEntityAxisY{0x30};
constexpr std::uintptr_t kEntityAxisZ{0x40};
constexpr std::uintptr_t kEntityPosition{0x50};
constexpr std::uintptr_t kEntityAabbMin{0x6C};
constexpr std::uintptr_t kEntityAabbMax{0x78};
constexpr std::uintptr_t kPawnIsDead{0x258};
constexpr std::uintptr_t kPawnArmy{0x284};
constexpr std::uintptr_t kPawnFaction{0x290};

// 0 Blue, 1 Red, 2 Grey, 3 Neutral, 4 Green, 5 Yellow.
constexpr std::int32_t kMaxArmy{5};
constexpr std::int32_t kMaxFaction{3};

constexpr std::uintptr_t kPawnOwnArmyBit{0x280};
constexpr std::uintptr_t kPawnHostileMask{0x281};
constexpr std::uintptr_t kPawnReverseHostile{0x283};

// Template and system entities all sit here with an identity transform.
constexpr float kTemplateCoordinate{-5248.0F};
constexpr float kTemplateTolerance{1.0F};
constexpr float kMaxWorldCoordinate{100000.0F};
constexpr float kMinHeight{0.2F};
constexpr float kMaxHeight{20.0F};

constexpr float kUnitTolerance{0.05F};
constexpr float kAxisTolerance{0.05F};
constexpr float kMaxBoxSpan{500.0F};
constexpr float kMinFov{0.1F};
constexpr float kMaxFov{3.0F};

// Far Cry 3 is 32-bit, so its stored pointers are four bytes wide whatever this
// build's uintptr_t is.
using EnginePtr = std::uint32_t;

[[nodiscard]] std::uintptr_t CachedModuleBase() noexcept {
  static std::uintptr_t base{0};
  if (base != 0) {
    return base;
  }

  const std::expected<std::uintptr_t, std::error_code> kBase{
      process::ModuleBase(kModuleName)};
  if (kBase) {
    base = *kBase;
  }

  return base;
}

[[nodiscard]] std::expected<std::uintptr_t, std::error_code> ReadPointer(
    std::uintptr_t address) noexcept {
  const std::expected<EnginePtr, std::error_code> kValue{
      process::ReadAt<EnginePtr>(address)};
  if (!kValue) {
    return std::unexpected{kValue.error()};
  }

  return static_cast<std::uintptr_t>(*kValue);
}

[[nodiscard]] bool IsUnitLength(const Triple& axis) noexcept {
  const float kLength{std::sqrt((axis.at(0) * axis.at(0)) +
                                (axis.at(1) * axis.at(1)) +
                                (axis.at(2) * axis.at(2)))};

  return std::fabs(kLength - 1.0F) < kUnitTolerance;
}

[[nodiscard]] Triple Cross(const Triple& lhs, const Triple& rhs) noexcept {
  return Triple{(lhs.at(1) * rhs.at(2)) - (lhs.at(2) * rhs.at(1)),
                (lhs.at(2) * rhs.at(0)) - (lhs.at(0) * rhs.at(2)),
                (lhs.at(0) * rhs.at(1)) - (lhs.at(1) * rhs.at(0))};
}

void FillViewport(Diagnostics& report) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kBlock{
      ReadPointer(report.viewportAddress)};
  if (!kBlock) {
    return;
  }
  report.viewportBlock = *kBlock;

  const std::expected<std::int32_t, std::error_code> kWidth{
      process::ReadAt<std::int32_t>(report.viewportBlock + kViewportWidth)};
  const std::expected<std::int32_t, std::error_code> kHeight{
      process::ReadAt<std::int32_t>(report.viewportBlock + kViewportHeight)};
  if (!kWidth || !kHeight) {
    return;
  }

  report.viewportWidth = *kWidth;
  report.viewportHeight = *kHeight;
}

void FillCameraChain(Diagnostics& report) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kContainer{
      ReadPointer(report.containerAddress)};
  if (!kContainer) {
    return;
  }
  report.container = *kContainer;

  const std::expected<std::int32_t, std::error_code> kLive{
      process::ReadAt<std::int32_t>(report.container + kContainerLiveFlag)};
  if (!kLive || *kLive == 0) {
    return;
  }
  report.liveFlag = *kLive;

  const std::expected<std::uintptr_t, std::error_code> kManagerSlot{
      ReadPointer(report.container + kContainerManagerSlot)};
  if (!kManagerSlot) {
    return;
  }

  const std::expected<std::uintptr_t, std::error_code> kManager{
      ReadPointer(*kManagerSlot)};
  if (!kManager) {
    return;
  }
  report.manager = *kManager;

  const std::expected<std::uintptr_t, std::error_code> kViewBase{
      ReadPointer(report.manager + kManagerViewSlot)};
  if (!kViewBase || *kViewBase == 0) {
    return;
  }
  report.cameraView = *kViewBase + kViewOffset;
}

// A full open-addressed table reports every further entity as already seen, so
// the tail of the frame would be dropped: keep the load factor near a quarter.
constexpr std::size_t kSeenSlots{8192};
// Engine allocations are 16-byte aligned, so the low bits carry no information.
constexpr std::uint32_t kAlignmentBits{4};

// The same entity turns up in several update passes; inspect it once a frame.
[[nodiscard]] std::array<EnginePtr, kSeenSlots>& Seen() noexcept {
  static std::array<EnginePtr, kSeenSlots> seen{};
  return seen;
}

[[nodiscard]] bool AlreadySeen(std::uintptr_t entity) noexcept {
  const auto kKey{static_cast<EnginePtr>(entity)};
  std::size_t slot{(kKey >> kAlignmentBits) % kSeenSlots};

  for (std::size_t probe{0}; probe < kSeenSlots; ++probe) {
    EnginePtr& cell{Seen().at(slot)};
    if (cell == 0) {
      cell = kKey;
      return false;
    }
    if (cell == kKey) {
      return true;
    }
    slot = (slot + 1) % kSeenSlots;
  }

  return true;
}

EntitySnapshot& PendingSnapshot() noexcept {
  static EntitySnapshot snapshot{};
  return snapshot;
}

EntitySnapshot& PublishedSnapshot() noexcept {
  static EntitySnapshot snapshot{};
  return snapshot;
}

[[nodiscard]] bool IsInTheWorld(const Triple& position) noexcept {
  for (const float kAxis : position) {
    if (!std::isfinite(kAxis) || std::fabs(kAxis) > kMaxWorldCoordinate) {
      return false;
    }
  }

  return std::fabs(position.at(0) - kTemplateCoordinate) > kTemplateTolerance ||
         std::fabs(position.at(1) - kTemplateCoordinate) > kTemplateTolerance;
}

[[nodiscard]] float Dot(const Triple& lhs, const Triple& rhs) noexcept {
  return (lhs.at(0) * rhs.at(0)) + (lhs.at(1) * rhs.at(1)) +
         (lhs.at(2) * rhs.at(2));
}

// The update lists also hold components, whose +0x50 is not a position. Only an
// entity has a rigid transform at +0x20, so this both identifies one and
// validates the numbers the box is built from.
[[nodiscard]] bool IsOrthonormal(const Triple& axisX, const Triple& axisY,
                                 const Triple& axisZ) noexcept {
  if (!IsUnitLength(axisX) || !IsUnitLength(axisY) || !IsUnitLength(axisZ)) {
    return false;
  }

  return std::fabs(Dot(axisX, axisY)) < kAxisTolerance &&
         std::fabs(Dot(axisX, axisZ)) < kAxisTolerance &&
         std::fabs(Dot(axisY, axisZ)) < kAxisTolerance;
}

[[nodiscard]] bool IsSaneBox(const Triple& low, const Triple& high) noexcept {
  for (std::size_t axis{0}; axis < kAxisCount; ++axis) {
    const float kSpan{high.at(axis) - low.at(axis)};
    if (!std::isfinite(kSpan) || kSpan < 0.0F || kSpan > kMaxBoxSpan) {
      return false;
    }
  }

  return true;
}

// A humanoid's object-space box is min (-0.6,-0.6,0), max (+0.6,+0.6,+1.70).
constexpr float kHumanoidMinHeight{1.3F};
constexpr float kHumanoidMaxHeight{2.3F};
constexpr float kHumanoidMaxWidth{1.5F};
constexpr float kHumanoidFootTolerance{0.4F};

[[nodiscard]] bool IsHumanoidBox(const Triple& low,
                                 const Triple& high) noexcept {
  return high.at(2) > kHumanoidMinHeight && high.at(2) < kHumanoidMaxHeight &&
         std::fabs(low.at(2)) < kHumanoidFootTolerance &&
         std::fabs(high.at(0)) < kHumanoidMaxWidth &&
         std::fabs(high.at(1)) < kHumanoidMaxWidth;
}

// The army fields sit past the 0xDC of a plain CEntity, so nothing is accepted
// unless the engine's invariant holds: the cached bit is exactly 1 << army.
void FillPawnFields(std::uintptr_t entity, EntityInfo& info) noexcept {
  const std::expected<std::int32_t, std::error_code> kArmy{
      process::ReadAt<std::int32_t>(entity + kPawnArmy)};
  const std::expected<std::uint8_t, std::error_code> kOwnBit{
      process::ReadAt<std::uint8_t>(entity + kPawnOwnArmyBit)};
  if (!kArmy || !kOwnBit || *kArmy < 0 || *kArmy > kMaxArmy) {
    return;
  }
  if (static_cast<std::uint32_t>(*kOwnBit) !=
      (1U << static_cast<std::uint32_t>(*kArmy))) {
    return;
  }

  const std::expected<std::int32_t, std::error_code> kFaction{
      process::ReadAt<std::int32_t>(entity + kPawnFaction)};
  if (!kFaction || *kFaction < 0 || *kFaction > kMaxFaction) {
    return;
  }

  info.pawn = true;
  info.army = *kArmy;
  info.faction = *kFaction;
  info.ownArmyBit = *kOwnBit;
  info.hostileMask =
      process::ReadAt<std::uint8_t>(entity + kPawnHostileMask).value_or(0);
  info.reverseHostileMask =
      process::ReadAt<std::uint8_t>(entity + kPawnReverseHostile).value_or(0);

  const std::expected<std::uint8_t, std::error_code> kDead{
      process::ReadAt<std::uint8_t>(entity + kPawnIsDead)};
  if (kDead) {
    info.deadByte = *kDead;
    info.dead = *kDead != 0;
  }
}

[[nodiscard]] bool FillGeometry(std::uintptr_t entity,
                                EntityInfo& info) noexcept {
  const std::expected<Triple, std::error_code> kAxisX{
      process::ReadAt<Triple>(entity + kEntityAxisX)};
  const std::expected<Triple, std::error_code> kAxisY{
      process::ReadAt<Triple>(entity + kEntityAxisY)};
  const std::expected<Triple, std::error_code> kAxisZ{
      process::ReadAt<Triple>(entity + kEntityAxisZ)};
  if (!kAxisX || !kAxisY || !kAxisZ ||
      !IsOrthonormal(*kAxisX, *kAxisY, *kAxisZ)) {
    return false;
  }

  const std::expected<Triple, std::error_code> kAabbMin{
      process::ReadAt<Triple>(entity + kEntityAabbMin)};
  const std::expected<Triple, std::error_code> kAabbMax{
      process::ReadAt<Triple>(entity + kEntityAabbMax)};
  if (!kAabbMin || !kAabbMax || !IsSaneBox(*kAabbMin, *kAabbMax)) {
    return false;
  }

  info.axisX = *kAxisX;
  info.axisY = *kAxisY;
  info.axisZ = *kAxisZ;
  info.aabbMin = *kAabbMin;
  info.aabbMax = *kAabbMax;
  info.humanoid = IsHumanoidBox(*kAabbMin, *kAabbMax);

  const float kSpan{kAabbMax->at(2) - kAabbMin->at(2)};
  if (kSpan > kMinHeight && kSpan < kMaxHeight) {
    info.height = kSpan;
  }

  return true;
}

enum class Verdict : std::uint8_t { NotEntity, NotInTheWorld, Stored };

[[nodiscard]] Verdict InspectEntity(std::uintptr_t entity,
                                    EntityInfo& info) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kVtable{
      ReadPointer(entity)};
  if (!kVtable || *kVtable == 0) {
    return Verdict::NotEntity;
  }

  if (!FillGeometry(entity, info)) {
    return Verdict::NotEntity;
  }

  const std::expected<Triple, std::error_code> kPosition{
      process::ReadAt<Triple>(entity + kEntityPosition)};
  if (!kPosition || !IsInTheWorld(*kPosition)) {
    return Verdict::NotInTheWorld;
  }

  info.address = entity;
  info.vtable = *kVtable;
  info.position = *kPosition;

  if (info.humanoid) {
    FillPawnFields(entity, info);
  }

  return Verdict::Stored;
}

EntityCensus& Pending() noexcept {
  static EntityCensus census{};
  return census;
}

EntityCensus& Published() noexcept {
  static EntityCensus census{};
  return census;
}

const hook::VmtHook*& BoundUpdate() noexcept {
  static const hook::VmtHook* hook{nullptr};
  return hook;
}

[[nodiscard]] std::uint32_t CollectGroup(std::uintptr_t group) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kEntityGroup{
      ReadPointer(group + kGroupEntityGroup)};
  if (!kEntityGroup || *kEntityGroup == 0) {
    return 0;
  }

  const std::expected<std::uintptr_t, std::error_code> kArray{
      ReadPointer(*kEntityGroup + kEntityGroupArray)};
  const std::expected<std::uint32_t, std::error_code> kCount{
      process::ReadAt<std::uint32_t>(*kEntityGroup + kEntityGroupCount)};
  if (!kArray || !kCount || *kArray == 0 || *kCount > kMaxGroupEntities) {
    return 0;
  }

  EntitySnapshot& snapshot{PendingSnapshot()};

  for (std::uint32_t index{0}; index < *kCount; ++index) {
    if (snapshot.count >= kMaxEntities) {
      snapshot.dropped += *kCount - index;
      break;
    }

    const std::expected<std::uintptr_t, std::error_code> kEntity{
        ReadPointer(*kArray + (index * sizeof(EnginePtr)))};
    if (!kEntity || *kEntity == 0 || AlreadySeen(*kEntity)) {
      continue;
    }

    EntityInfo info{};
    const Verdict kVerdict{InspectEntity(*kEntity, info)};
    if (kVerdict == Verdict::NotEntity) {
      ++snapshot.notEntities;
      continue;
    }
    if (kVerdict != Verdict::Stored) {
      continue;
    }

    snapshot.items.at(snapshot.count) = info;
    ++snapshot.count;
    if (info.pawn) {
      ++snapshot.pawns;
    }
    if (info.humanoid) {
      ++snapshot.humanoids;
    }
  }

  return *kCount;
}

}  // namespace

std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept {
  const std::uintptr_t kBase{CachedModuleBase()};
  if (kBase == 0) {
    return 0;
  }

  return kBase + (ghidraAddress - kGhidraImageBase);
}

Diagnostics ReadDiagnostics() noexcept {
  Diagnostics report{};

  const std::uintptr_t kBase{CachedModuleBase()};
  if (kBase == 0) {
    return report;
  }

  report.moduleBase = kBase;
  report.containerAddress = Resolve(kCameraManagerContainer);
  report.viewportAddress = Resolve(kViewportBlock);

  FillViewport(report);
  FillCameraChain(report);

  return report;
}

std::expected<CameraBasis, std::error_code> ReadCamera() noexcept {
  const Diagnostics kReport{ReadDiagnostics()};
  if (kReport.cameraView == 0) {
    return std::unexpected{make_error_code(Error::CameraUnavailable)};
  }
  const std::expected<std::uintptr_t, std::error_code> kView{
      kReport.cameraView};

  const std::expected<Triple, std::error_code> kPosition{
      process::ReadAt<Triple>(*kView + kViewPosition)};
  const std::expected<Triple, std::error_code> kForward{
      process::ReadAt<Triple>(*kView + kViewForward)};
  const std::expected<Triple, std::error_code> kUp{
      process::ReadAt<Triple>(*kView + kViewUp)};
  const std::expected<float, std::error_code> kFov{
      process::ReadAt<float>(*kView + kViewVerticalFov)};
  const std::expected<float, std::error_code> kFovHorizontal{
      process::ReadAt<float>(*kView + kViewHorizontalFov)};
  if (!kPosition || !kForward || !kUp || !kFov || !kFovHorizontal) {
    return std::unexpected{make_error_code(Error::CameraNotFound)};
  }

  if (!IsUnitLength(*kForward) || !IsUnitLength(*kUp) || *kFov < kMinFov ||
      *kFov > kMaxFov || *kFovHorizontal < kMinFov ||
      *kFovHorizontal > kMaxFov) {
    return std::unexpected{make_error_code(Error::CameraNotSane)};
  }

  // The engine keeps its own right vector next to the basis; the cross product
  // is only the fallback.
  const std::expected<Triple, std::error_code> kRight{
      process::ReadAt<Triple>(*kView + kViewRight)};

  return CameraBasis{.position = *kPosition,
                     .forward = *kForward,
                     .upAxis = *kUp,
                     .right = (kRight && IsUnitLength(*kRight))
                                  ? *kRight
                                  : Cross(*kForward, *kUp),
                     .fovRadians = *kFov,
                     .fovHorizontalRadians = *kFovHorizontal};
}

void** RunAllGroupSlot() noexcept {
  return std::bit_cast<void**>(Resolve(kRunAllGroupSlotAddress));
}

void TakeCensus(std::uintptr_t scheduler) noexcept {
  EntityCensus& census{Pending()};
  ++census.calls;

  const std::expected<std::uintptr_t, std::error_code> kGroups{
      ReadPointer(scheduler + kSchedulerGroups)};
  const std::expected<std::uint32_t, std::error_code> kGroupCount{
      process::ReadAt<std::uint32_t>(scheduler + kSchedulerGroupCount)};
  if (!kGroups || !kGroupCount || *kGroups == 0) {
    return;
  }

  const std::uint32_t kWanted{*kGroupCount};
  const std::uint32_t kVisited{kWanted < kMaxGroups ? kWanted : kMaxGroups};
  census.skipped += kWanted - kVisited;

  for (std::uint32_t index{0}; index < kVisited; ++index) {
    const std::expected<std::uintptr_t, std::error_code> kGroup{
        ReadPointer(*kGroups + (index * sizeof(EnginePtr)))};
    if (!kGroup || *kGroup == 0) {
      continue;
    }

    const std::uint32_t kEntities{CollectGroup(*kGroup)};
    if (kEntities == 0) {
      continue;
    }

    ++census.groups;
    census.entities += kEntities;
  }
}

void PublishCensus() noexcept {
  EntityCensus census{Pending()};
  census.frames = Published().frames + 1;

  Published() = census;
  Pending() = EntityCensus{};

  PublishedSnapshot() = PendingSnapshot();
  PendingSnapshot().count = 0;
  PendingSnapshot().dropped = 0;
  PendingSnapshot().pawns = 0;
  PendingSnapshot().humanoids = 0;
  PendingSnapshot().notEntities = 0;
  Seen().fill(0);
  process::ForgetReadableRegions();
}

EntityCensus LastCensus() noexcept { return Published(); }

const EntitySnapshot& LastSnapshot() noexcept { return PublishedSnapshot(); }

UpdateBinding::UpdateBinding(const hook::VmtHook& hook) noexcept {
  BoundUpdate() = &hook;
}

UpdateBinding::~UpdateBinding() { BoundUpdate() = nullptr; }

RunAllGroupFn OriginalRunAllGroup() noexcept {
  const hook::VmtHook* const kHook{BoundUpdate()};
  if (kHook == nullptr) {
    return nullptr;
  }

  return kHook->Original<RunAllGroupFn>();
}

std::expected<Viewport, std::error_code> ReadViewport() noexcept {
  const Diagnostics kReport{ReadDiagnostics()};
  if (kReport.viewportWidth <= 0 || kReport.viewportHeight <= 0) {
    return std::unexpected{make_error_code(Error::ViewportNotSane)};
  }

  return Viewport{.width = static_cast<float>(kReport.viewportWidth),
                  .height = static_cast<float>(kReport.viewportHeight)};
}

}  // namespace afw::games::fc3
