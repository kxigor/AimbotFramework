#pragma once

#include <gtest/gtest.h>
#include <windows.h>

#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>

namespace afw::testing {

// A fake world laid out the way the engine lays out a real one, so the walk
// under test reads it with the very offsets it uses on the game. The offsets
// come from internal/layout.hpp rather than from copies kept here: a test that
// carries its own copy of an offset passes after the real one is corrected.

constexpr SIZE_T kBufferBytes{16384};

// The engine's pointers are four bytes wide, so the fake has to live where a
// 32-bit value can address it. On a 64-bit test run a plain allocation would
// not.
constexpr std::array<std::uintptr_t, 3> kPreferredBases{0x20000000, 0x30000000,
                                                        0x40000000};

// Addresses inside the fake page. These are the fake's own shape and belong to
// the test, unlike the field offsets, which belong to the binary.
constexpr std::uint32_t kScheduler{0x000};
constexpr std::uint32_t kGroupArray{0x040};
constexpr std::uint32_t kGroupOne{0x080};
constexpr std::uint32_t kGroupTwo{0x0C0};
constexpr std::uint32_t kEntityGroupOne{0x100};
constexpr std::uint32_t kEntityGroupTwo{0x140};
constexpr std::uint32_t kEntityArrayOne{0x200};
constexpr std::uint32_t kEntityArrayTwo{0x240};

constexpr std::uint32_t kEntitiesOne{7};
constexpr std::uint32_t kEntitiesTwo{5};

// Two fake entities, far enough apart that the pawn fields of the first do not
// reach into the second.
constexpr std::uint32_t kEntityOne{0x400};
constexpr std::uint32_t kEntityTwo{0x800};

constexpr std::uint32_t kComponentArrayOne{0x1000};
constexpr std::uint32_t kComponentOne{0x1100};
constexpr std::uint32_t kAgentOne{0x2000};

constexpr std::uint32_t kRowBytes{0x10};
constexpr std::uint32_t kVec3Bytes{0x0C};
constexpr std::uintptr_t kWorldWrap{0x10000};

class Fc3World : public ::testing::Test {
 protected:
  void SetUp() override {
    for (const std::uintptr_t kBase : kPreferredBases) {
      page_ = VirtualAlloc(
          /*lpAddress        = */ std::bit_cast<LPVOID>(kBase),
          /*dwSize           = */ kBufferBytes,
          /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
          /*flProtect        = */ PAGE_READWRITE);
      if (page_ != nullptr) {
        break;
      }
    }
    if (page_ == nullptr) {
      GTEST_SKIP() << "no free reservation below 4 GiB for the fake scheduler";
    }
    base_ = static_cast<std::uint32_t>(std::bit_cast<std::uintptr_t>(page_));

    Poke(kScheduler + games::fc3::kSchedulerGroups, At(kGroupArray));
    Poke(kScheduler + games::fc3::kSchedulerGroupCount, 2);

    Poke(kGroupArray, At(kGroupOne));
    Poke(kGroupArray + sizeof(std::uint32_t), At(kGroupTwo));

    Poke(kGroupOne + games::fc3::kGroupEntityGroup, At(kEntityGroupOne));
    Poke(kGroupTwo + games::fc3::kGroupEntityGroup, At(kEntityGroupTwo));

    Poke(kEntityGroupOne + games::fc3::kEntityGroupArray, At(kEntityArrayOne));
    Poke(kEntityGroupOne + games::fc3::kEntityGroupCount, kEntitiesOne);

    Poke(kEntityGroupTwo + games::fc3::kEntityGroupArray, At(kEntityArrayTwo));
    Poke(kEntityGroupTwo + games::fc3::kEntityGroupCount, kEntitiesTwo);

    GiveAnAgent(kEntityOne, kComponentArrayOne, kComponentOne, kAgentOne);

    // The census keeps its buffers in statics that outlive a test, so where a
    // generation stands depends on what ran before. Publishing once here asks
    // for a new one, which makes the next walk open a fresh generation
    // whatever the previous test left behind.
    games::fc3::PublishCensus();
  }

  void TearDown() override {
    if (page_ != nullptr) {
      VirtualFree(
          /*lpAddress  = */ page_,
          /*dwSize     = */ 0,
          /*dwFreeType = */ MEM_RELEASE);
    }
  }

  // Only the first entity gets one, so a test can tell the two apart by whether
  // a pawn record appeared at all.
  void GiveAnAgent(std::uint32_t entity, std::uint32_t array,
                   std::uint32_t component, std::uint32_t agent) {
    Poke(entity + games::fc3::kEntityComponentArray, At(array));
    Poke(entity + games::fc3::kEntityComponentCount, 1);
    Poke(array, At(component));
    Poke(component, 0xC0FF0000 + component);
    Poke(component + games::fc3::kComponentAgent, At(agent));
    Poke(agent, 0xA6E70000 + agent);
  }

  void Poke(std::uintptr_t offset, std::uint32_t value) {
    std::memcpy(static_cast<std::byte*>(page_) + offset, &value, sizeof(value));
  }

  void PokeFloat(std::uintptr_t offset, float value) {
    std::memcpy(static_cast<std::byte*>(page_) + offset, &value, sizeof(value));
  }

  // A rigid identity basis with the position in row 3, and an object-space box
  // around the origin. Everything Classify insists on, and nothing else.
  void PlaceEntity(std::uint32_t entity, float x, float y, float halfWidth,
                   float height) {
    Poke(entity, 0xABCD0000 + entity);

    const std::uintptr_t kTransform{entity + games::fc3::kEntityTransform};
    PokeFloat(kTransform, 1.0F);
    PokeFloat(kTransform + kRowBytes + 4, 1.0F);
    PokeFloat(kTransform + (2 * kRowBytes) + 8, 1.0F);
    PokeFloat(kTransform + (3 * kRowBytes) + 12, 1.0F);

    PokeFloat(kTransform + (3 * kRowBytes), x);
    PokeFloat(kTransform + (3 * kRowBytes) + 4, y);
    PokeFloat(kTransform + (3 * kRowBytes) + 8, 0.0F);

    const std::uintptr_t kBox{entity + games::fc3::kEntityAabb};
    PokeFloat(kBox, -halfWidth);
    PokeFloat(kBox + 4, -halfWidth);
    PokeFloat(kBox + 8, 0.0F);
    PokeFloat(kBox + kVec3Bytes, halfWidth);
    PokeFloat(kBox + kVec3Bytes + 4, halfWidth);
    PokeFloat(kBox + kVec3Bytes + 8, height);
  }

  [[nodiscard]] std::uint32_t At(std::uint32_t offset) const {
    return base_ + offset;
  }

  // A generation is closed by the walk that follows the frame asking for one,
  // so seeing what a walk found takes a second frame. The engine gets that for
  // free; a test has to spell it out.
  static void Settle(std::uint32_t scheduler) {
    games::fc3::PublishCensus();
    games::fc3::TakeCensus(scheduler);
    games::fc3::PublishCensus();
  }

  [[nodiscard]] static games::fc3::EntityCensus Walk(std::uint32_t scheduler) {
    games::fc3::TakeCensus(scheduler);
    Settle(scheduler);

    return games::fc3::LastCensus();
  }

  LPVOID page_{nullptr};
  std::uint32_t base_{0};
};

}  // namespace afw::testing
