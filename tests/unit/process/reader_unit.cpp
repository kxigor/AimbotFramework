#include <gtest/gtest.h>
#include <windows.h>

#include <afw/process/reader.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>

namespace {

// The engine this vocabulary exists for is a 32-bit process, so the pointers it
// stores are four bytes wide however wide this build's uintptr_t is. The fake
// therefore has to live where a 32-bit value can address it, which a plain
// allocation on a 64-bit test run would not.
using Engine = afw::process::Reader<std::uint32_t>;
using EngineArray = afw::process::PointerArray<std::uint32_t>;

constexpr SIZE_T kBufferBytes{4096};
constexpr std::array<std::uintptr_t, 3> kPreferredBases{0x50000000, 0x60000000,
                                                        0x70000000};

constexpr std::uint32_t kOwner{0x000};
constexpr std::uint32_t kArrayOffset{0x10};
constexpr std::uint32_t kCountOffset{0x14};
constexpr std::uint32_t kArray{0x100};
constexpr std::uint32_t kFirst{0x200};
constexpr std::uint32_t kSecond{0x280};
constexpr std::uint32_t kLimit{64};

class ProcessReaderUnit : public ::testing::Test {
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
      GTEST_SKIP() << "no free reservation below 4 GiB for the fake object";
    }
    base_ = static_cast<std::uint32_t>(std::bit_cast<std::uintptr_t>(page_));

    Poke(kOwner + kArrayOffset, At(kArray));
    Poke(kOwner + kCountOffset, 2);
    Poke(kArray, At(kFirst));
    Poke(kArray + sizeof(std::uint32_t), At(kSecond));
  }

  void TearDown() override {
    if (page_ != nullptr) {
      VirtualFree(
          /*lpAddress  = */ page_,
          /*dwSize     = */ 0,
          /*dwFreeType = */ MEM_RELEASE);
    }
  }

  void Poke(std::uintptr_t offset, std::uint32_t value) {
    std::memcpy(static_cast<std::byte*>(page_) + offset, &value, sizeof(value));
  }

  [[nodiscard]] std::uint32_t At(std::uint32_t offset) const {
    return base_ + offset;
  }

  [[nodiscard]] std::optional<EngineArray> Array() const {
    return EngineArray::At(At(kOwner), kArrayOffset, kCountOffset, kLimit);
  }

  LPVOID page_{nullptr};
  std::uint32_t base_{0};
};

TEST_F(ProcessReaderUnit, ReadsAFieldAtAnOffset) {
  Poke(0x20, 0xFEEDU);

  EXPECT_EQ(Engine{At(kOwner)}.Field<std::uint32_t>(0x20), 0xFEEDU);
}

// An unreadable address is an ordinary answer here, not an error: the walk is
// over objects whose class is not yet established, so a miss means "not this
// one".
TEST_F(ProcessReaderUnit, ReportsAnUnreadableAddressAsNothing) {
  EXPECT_FALSE(Engine{0}.Field<std::uint32_t>(0).has_value());
}

// Field keeps a zero and Pointer refuses one. The difference is load-bearing:
// a vtable slot of zero is a fact about the object, while a component pointer
// of zero is an empty slot to step over.
TEST_F(ProcessReaderUnit, FieldKeepsAZeroThatPointerRefuses) {
  Poke(0x30, 0);

  EXPECT_EQ(Engine{At(kOwner)}.Field<std::uint32_t>(0x30), 0U);
  EXPECT_FALSE(Engine{At(kOwner)}.Pointer(0x30).has_value());
}

TEST_F(ProcessReaderUnit, FieldOrAnswersTheFallbackRatherThanFailing) {
  EXPECT_EQ(Engine{0}.FieldOr<std::uint32_t>(0, 7U), 7U);
}

// The whole point of CountAtMost: a count misread out of a foreign object is
// what turns one wrong offset into a walk of the address space.
TEST_F(ProcessReaderUnit, RefusesACountBeyondTheLimit) {
  Poke(kOwner + kCountOffset, 100000);

  EXPECT_FALSE(
      Engine{At(kOwner)}.CountAtMost(kCountOffset, kLimit).has_value());
  EXPECT_FALSE(Array().has_value());
}

TEST_F(ProcessReaderUnit, AcceptsACountOfZeroAsAnEmptyArray) {
  Poke(kOwner + kCountOffset, 0);

  const std::optional<EngineArray> kArrayView{Array()};

  ASSERT_TRUE(kArrayView.has_value());
  EXPECT_EQ(kArrayView->Size(), 0U);
}

TEST_F(ProcessReaderUnit, BoundsTheCountOnceAndHandsOutEachSlot) {
  const std::optional<EngineArray> kArrayView{Array()};

  ASSERT_TRUE(kArrayView.has_value());
  ASSERT_EQ(kArrayView->Size(), 2U);
  EXPECT_EQ(kArrayView->Slot(0), At(kFirst));
  EXPECT_EQ(kArrayView->Slot(1), At(kSecond));
}

TEST_F(ProcessReaderUnit, StepsOverAnEmptySlot) {
  Poke(kArray, 0);

  const std::optional<EngineArray> kArrayView{Array()};

  ASSERT_TRUE(kArrayView.has_value());
  EXPECT_FALSE(kArrayView->Slot(0).has_value());
  EXPECT_EQ(kArrayView->Slot(1), At(kSecond));
}

TEST_F(ProcessReaderUnit, RefusesAnArrayWhoseBasePointerIsNull) {
  Poke(kOwner + kArrayOffset, 0);

  EXPECT_FALSE(Array().has_value());
}

}  // namespace
