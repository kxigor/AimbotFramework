#include <gtest/gtest.h>
#include <windows.h>

#include <afw/hook/error.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <bit>
#include <cstddef>
#include <expected>
#include <support/win_probe.hpp>
#include <system_error>

namespace {

constexpr SIZE_T kTableSize{4096};
constexpr std::size_t kSlot{0};

using ProbeFn = int (*)();

int OriginalFn() { return 1; }
int DetourFn() { return 2; }
int OtherDetourFn() { return 3; }

class VmtHookUnit : public ::testing::Test {
 protected:
  void SetUp() override {
    page_ = VirtualAlloc(
        /*lpAddress        = */ nullptr,
        /*dwSize           = */ kTableSize,
        /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
        /*flProtect        = */ PAGE_READWRITE);
    ASSERT_NE(page_, nullptr);

    table_ = static_cast<void**>(page_);
    table_[kSlot] = std::bit_cast<void*>(&OriginalFn);

    // A real vtable lives in .rdata, so make the probe read-only too --
    // otherwise ProtectGuard would have nothing to do and the tests would pass
    // even if the guard were removed.
    DWORD previous{};
    ASSERT_NE(VirtualProtect(
                  /*lpAddress      = */ page_,
                  /*dwSize         = */ kTableSize,
                  /*flNewProtect   = */ PAGE_READONLY,
                  /*lpflOldProtect = */ &previous),
              FALSE);
  }

  void TearDown() override {
    if (page_ != nullptr) {
      VirtualFree(
          /*lpAddress  = */ page_,
          /*dwSize     = */ 0,
          /*dwFreeType = */ MEM_RELEASE);
    }
  }

  [[nodiscard]] int CallSlot() const {
    return std::bit_cast<ProbeFn>(table_[kSlot])();
  }

  LPVOID page_{nullptr};
  void** table_{nullptr};
};

}  // namespace

TEST_F(VmtHookUnit, ConstructionPublishesTheOriginal) {
  const afw::hook::VmtHook hook{table_, kSlot};

  EXPECT_EQ(hook.Original<ProbeFn>()(), 1);
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookUnit, SetDetourDoesNotTouchTheSlot) {
  afw::hook::VmtHook hook{table_, kSlot};

  const std::expected<void, std::error_code> kSet{hook.SetDetour(&DetourFn)};

  ASSERT_TRUE(kSet.has_value()) << kSet.error().message();
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookUnit, SetDetourWhileEnabledCannotLoseTheOriginal) {
  afw::hook::VmtHook hook{table_, kSlot};
  ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());
  ASSERT_TRUE(hook.Enable().has_value());
  ASSERT_EQ(CallSlot(), 2);

  ASSERT_TRUE(hook.SetDetour(&OtherDetourFn).has_value());

  EXPECT_EQ(hook.Original<ProbeFn>()(), 1);
  ASSERT_TRUE(hook.Disable().has_value());
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookUnit, EnableRedirectsTheSlotToTheDetour) {
  afw::hook::VmtHook hook{table_, kSlot};
  ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());
  ASSERT_EQ(CallSlot(), 1);

  const std::expected<void, std::error_code> kEnabled{hook.Enable()};

  ASSERT_TRUE(kEnabled.has_value()) << kEnabled.error().message();
  EXPECT_EQ(CallSlot(), 2);
  EXPECT_EQ(hook.Original<ProbeFn>()(), 1);
}

TEST_F(VmtHookUnit, EnableLeavesThePageProtectionAsItFoundIt) {
  afw::hook::VmtHook hook{table_, kSlot};
  ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());

  ASSERT_TRUE(hook.Enable().has_value());

  EXPECT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                             PAGE_READONLY));
}

TEST_F(VmtHookUnit, DisableRestoresTheOriginal) {
  afw::hook::VmtHook hook{table_, kSlot};
  ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());
  ASSERT_TRUE(hook.Enable().has_value());
  ASSERT_EQ(CallSlot(), 2);

  const std::expected<void, std::error_code> kDisabled{hook.Disable()};

  ASSERT_TRUE(kDisabled.has_value()) << kDisabled.error().message();
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookUnit, DestructorRestoresTheOriginal) {
  {
    afw::hook::VmtHook hook{table_, kSlot};
    ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());
    ASSERT_TRUE(hook.Enable().has_value());
    ASSERT_EQ(CallSlot(), 2);
  }

  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookUnit, EnableWithoutADetourReportsDetourNotSet) {
  afw::hook::VmtHook hook{table_, kSlot};

  const std::expected<void, std::error_code> kEnabled{hook.Enable()};

  ASSERT_FALSE(kEnabled.has_value());
  EXPECT_EQ(kEnabled.error(),
            afw::hook::make_error_code(afw::hook::Error::DetourNotSet));
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookUnit, EnableTwiceReportsHookAlreadyApplied) {
  afw::hook::VmtHook hook{table_, kSlot};
  ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());
  ASSERT_TRUE(hook.Enable().has_value());

  const std::expected<void, std::error_code> kSecond{hook.Enable()};

  ASSERT_FALSE(kSecond.has_value());
  EXPECT_EQ(kSecond.error(),
            afw::hook::make_error_code(afw::hook::Error::HookAlreadyApplied));
  EXPECT_EQ(&kSecond.error().category(), &afw::hook::ErrorCategory());
  EXPECT_EQ(CallSlot(), 2);
}

TEST_F(VmtHookUnit, DisableWithoutEnableReportsHookNotApplied) {
  afw::hook::VmtHook hook{table_, kSlot};
  ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());

  const std::expected<void, std::error_code> kDisabled{hook.Disable()};

  ASSERT_FALSE(kDisabled.has_value());
  EXPECT_EQ(kDisabled.error(),
            afw::hook::make_error_code(afw::hook::Error::HookNotApplied));
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookUnit, EnableAfterDisableHooksAgain) {
  afw::hook::VmtHook hook{table_, kSlot};
  ASSERT_TRUE(hook.SetDetour(&DetourFn).has_value());
  ASSERT_TRUE(hook.Enable().has_value());
  ASSERT_TRUE(hook.Disable().has_value());

  ASSERT_TRUE(hook.Enable().has_value());

  EXPECT_EQ(CallSlot(), 2);
}

TEST_F(VmtHookUnit, SetDetourRejectsAnUnreadableSlot) {
  void** unmapped{nullptr};
  {
    const LPVOID kReserved{VirtualAlloc(
        /*lpAddress        = */ nullptr,
        /*dwSize           = */ kTableSize,
        /*flAllocationType = */ MEM_RESERVE,
        /*flProtect        = */ PAGE_NOACCESS)};
    ASSERT_NE(kReserved, nullptr);
    ASSERT_NE(VirtualFree(
                  /*lpAddress  = */ kReserved,
                  /*dwSize     = */ 0,
                  /*dwFreeType = */ MEM_RELEASE),
              FALSE);
    unmapped = static_cast<void**>(kReserved);
  }

  afw::hook::VmtHook hook{unmapped, kSlot};

  const std::expected<void, std::error_code> kSet{hook.SetDetour(&DetourFn)};

  ASSERT_FALSE(kSet.has_value());
  EXPECT_EQ(kSet.error(),
            afw::hook::make_error_code(afw::hook::Error::SlotNotReadable));
}
