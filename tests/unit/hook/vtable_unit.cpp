#include <gtest/gtest.h>

#include <afw/hook/vtable.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace {

constexpr std::size_t kEntries{4};
constexpr std::size_t kProbeIndex{2};

class VtableUnit : public ::testing::Test {
 protected:
  void SetUp() override {
    for (std::size_t i{0}; i < kEntries; ++i) {
      entries_[i] = &entries_[i];
    }
  }

  [[nodiscard]] afw::hook::Vtable Table() {
    return afw::hook::Vtable{entries_.data()};
  }

  std::array<void*, kEntries> entries_{};
};

}  // namespace

TEST_F(VtableUnit, GetReturnsTheTableItWasGiven) {
  EXPECT_EQ(Table().Get(), entries_.data());
}

TEST_F(VtableUnit, SlotZeroIsTheTableItself) {
  EXPECT_EQ(Table().Slot(0), Table().Get());
}

TEST_F(VtableUnit, SlotReturnsTheAddressOfTheEntry) {
  EXPECT_EQ(Table().Slot(kProbeIndex), &entries_[kProbeIndex]);
}

TEST_F(VtableUnit, AtReturnsTheValueOfTheEntry) {
  EXPECT_EQ(Table().At(kProbeIndex), entries_[kProbeIndex]);
}

TEST_F(VtableUnit, AtAgreesWithSlot) {
  const afw::hook::Vtable kTable{Table()};

  for (std::size_t i{0}; i < kEntries; ++i) {
    EXPECT_EQ(kTable.At(i), *kTable.Slot(i)) << "slot " << i;
  }
}

TEST_F(VtableUnit, AdjacentSlotsAreOnePointerApart) {
  const afw::hook::Vtable kTable{Table()};

  const std::uintptr_t kFirst{std::bit_cast<std::uintptr_t>(kTable.Slot(0))};
  const std::uintptr_t kSecond{std::bit_cast<std::uintptr_t>(kTable.Slot(1))};

  EXPECT_EQ(kSecond - kFirst, sizeof(void*));
}
