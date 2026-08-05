#include <gtest/gtest.h>

#include <afw/games/fc3/fc3.hpp>
#include <atomic>
#include <cstdint>
#include <support/fc3_world.hpp>
#include <thread>

namespace {

using afw::testing::Fc3World;
using afw::testing::kEntityArrayOne;
using afw::testing::kEntityArrayTwo;
using afw::testing::kEntityOne;
using afw::testing::kEntityTwo;
using afw::testing::kScheduler;
using afw::testing::kWorldWrap;

// ThreadSanitizer does not exist for this target, and the other suites are
// single-threaded, so nothing there can observe the race the triple buffer
// exists to remove. This one creates it: the walk runs on its own thread while
// the reader publishes and inspects. Each entity's position is set from its own
// address, so a record stitched together from two different entities -- which
// is what the old shared-buffer handoff produced -- fails the comparison.
class Fc3HandoffUnit : public Fc3World {
 protected:
  void SetUp() override {
    Fc3World::SetUp();
    if (page_ == nullptr) {
      return;
    }

    PlaceEntity(kEntityOne, PositionFor(At(kEntityOne)), 0.0F, 2.0F, 5.0F);
    PlaceEntity(kEntityTwo, PositionFor(At(kEntityTwo)), 0.0F, 2.0F, 5.0F);

    Poke(kEntityArrayTwo, At(kEntityTwo));

    // The census statics outlive a test, and the fake page keeps the same base
    // address, so the dedupe table can still hold these very entities from an
    // earlier test. Closing one generation here clears it.
    Settle(At(kScheduler));
  }

  // The whole address would land far outside kMaxWorldCoordinate and be
  // rejected, so only the part that differs between the two entities is used.
  [[nodiscard]] static float PositionFor(std::uintptr_t address) {
    return static_cast<float>(address % kWorldWrap);
  }

  [[nodiscard]] static bool IsSelfConsistent(
      const afw::games::fc3::EntityInfo& entity) {
    return entity.transform.Translation().x == PositionFor(entity.address);
  }
};

// esp::DrawFrame holds the reference LastSnapshot() returns across five passes
// over the array and the whole ImGui build, which is milliseconds. So the
// invariant is not "a record is consistent when read" but "the buffer does not
// change underneath the reader for as long as it holds it".
TEST_F(Fc3HandoffUnit, TheHeldSnapshotDoesNotChangeUnderTheReader) {
  constexpr int kWalks{20000};
  constexpr int kYields{64};

  std::atomic<bool> walking{true};
  std::thread writer{[this, &walking] {
    for (int walk{0}; walk < kWalks; ++walk) {
      Poke(kEntityArrayOne, (walk % 2) == 0 ? At(kEntityOne) : At(kEntityTwo));
      afw::games::fc3::TakeCensus(At(kScheduler));
    }
    walking.store(false);
  }};

  std::uint32_t inspected{0};
  std::uint32_t torn{0};
  std::uint32_t moved{0};
  while (walking.load()) {
    afw::games::fc3::PublishCensus();

    const afw::games::fc3::EntitySnapshot& kSnapshot{
        afw::games::fc3::LastSnapshot()};
    ASSERT_LE(kSnapshot.count, afw::games::fc3::kMaxEntities);
    if (kSnapshot.count == 0) {
      continue;
    }

    const std::uint32_t kCount{kSnapshot.count};
    const std::uintptr_t kAddress{kSnapshot.items.at(0).address};
    const float kX{kSnapshot.items.at(0).transform.Translation().x};

    for (int yield{0}; yield < kYields; ++yield) {
      std::this_thread::yield();
    }

    ++inspected;
    if (kSnapshot.count != kCount ||
        kSnapshot.items.at(0).address != kAddress ||
        kSnapshot.items.at(0).transform.Translation().x != kX) {
      ++moved;
    }
    for (std::uint32_t index{0}; index < kSnapshot.count; ++index) {
      if (!IsSelfConsistent(kSnapshot.items.at(index))) {
        ++torn;
      }
    }
  }

  writer.join();

  EXPECT_EQ(moved, 0U) << "the buffer changed while the reader held it";
  EXPECT_EQ(torn, 0U);
  EXPECT_GT(inspected, 0U) << "the reader never observed a published snapshot";
}

}  // namespace
