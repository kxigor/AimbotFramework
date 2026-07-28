#include <gtest/gtest.h>
#include <windows.h>

#include <afw/hook/error.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <cstddef>
#include <cstring>
#include <expected>
#include <support/WinProbe.hpp>
#include <system_error>

namespace {

constexpr SIZE_T kTableSize{4096};
constexpr std::size_t kSlot{0};

using ProbeFn = int (*)();

int OriginalFn() { return 1; }
int DetourFn() { return 2; }

// ISO C++ does not allow reinterpret_cast between function and object
// pointers, and -Wpedantic is on. memcpy is the sanctioned way across.
[[nodiscard]] void* ToPointer(ProbeFn fn) {
  void* result{nullptr};
  std::memcpy(&result, &fn, sizeof(result));
  return result;
}

[[nodiscard]] ProbeFn ToFunction(void* pointer) {
  ProbeFn result{nullptr};
  std::memcpy(&result, &pointer, sizeof(result));
  return result;
}

class VmtHookTest : public ::testing::Test {
 protected:
  void SetUp() override {
    page_ = VirtualAlloc(
        /*lpAddress        = */ nullptr,
        /*dwSize           = */ kTableSize,
        /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
        /*flProtect        = */ PAGE_READWRITE);
    ASSERT_NE(page_, nullptr);

    table_ = static_cast<void**>(page_);
    table_[kSlot] = ToPointer(&OriginalFn);

    // A real vtable lives in .rdata, so make the probe read-only too --
    // otherwise ProtectGuard would have nothing to do and the test would pass
    // even if the guard were removed.
    DWORD previous{0};
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

  [[nodiscard]] int CallSlot() const { return ToFunction(table_[kSlot])(); }

  LPVOID page_{nullptr};
  void** table_{nullptr};
};

}  // namespace

TEST_F(VmtHookTest, EnableRedirectsTheSlotToTheDetour) {
  afw::hook::VmtHook hook{table_, kSlot, ToPointer(&DetourFn)};
  ASSERT_EQ(CallSlot(), 1);

  const std::expected<void, std::error_code> kEnabled{hook.Enable()};

  ASSERT_TRUE(kEnabled.has_value()) << kEnabled.error().message();
  EXPECT_EQ(CallSlot(), 2);
}

TEST_F(VmtHookTest, EnableLeavesThePageProtectionAsItFoundIt) {
  afw::hook::VmtHook hook{table_, kSlot, ToPointer(&DetourFn)};

  ASSERT_TRUE(hook.Enable().has_value());

  EXPECT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                             PAGE_READONLY));
}

TEST_F(VmtHookTest, DisableRestoresTheOriginal) {
  afw::hook::VmtHook hook{table_, kSlot, ToPointer(&DetourFn)};
  ASSERT_TRUE(hook.Enable().has_value());
  ASSERT_EQ(CallSlot(), 2);

  const std::expected<void, std::error_code> kDisabled{hook.Disable()};

  ASSERT_TRUE(kDisabled.has_value()) << kDisabled.error().message();
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookTest, DestructorRestoresTheOriginal) {
  {
    afw::hook::VmtHook hook{table_, kSlot, ToPointer(&DetourFn)};
    ASSERT_TRUE(hook.Enable().has_value());
    ASSERT_EQ(CallSlot(), 2);
  }

  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookTest, EnableTwiceReportsHookAlreadyApplied) {
  afw::hook::VmtHook hook{table_, kSlot, ToPointer(&DetourFn)};
  ASSERT_TRUE(hook.Enable().has_value());

  const std::expected<void, std::error_code> kSecond{hook.Enable()};

  ASSERT_FALSE(kSecond.has_value());
  EXPECT_EQ(kSecond.error(), afw::hook::make_error_code(
                                 afw::hook::Error::HookAlreadyApplied));
  EXPECT_EQ(&kSecond.error().category(), &afw::hook::ErrorCategory());
  EXPECT_EQ(CallSlot(), 2);
}

TEST_F(VmtHookTest, DisableWithoutEnableReportsHookNotApplied) {
  afw::hook::VmtHook hook{table_, kSlot, ToPointer(&DetourFn)};

  const std::expected<void, std::error_code> kDisabled{hook.Disable()};

  ASSERT_FALSE(kDisabled.has_value());
  EXPECT_EQ(kDisabled.error(),
            afw::hook::make_error_code(afw::hook::Error::HookNotApplied));
  EXPECT_EQ(CallSlot(), 1);
}

TEST_F(VmtHookTest, EnableAfterDisableHooksAgain) {
  afw::hook::VmtHook hook{table_, kSlot, ToPointer(&DetourFn)};
  ASSERT_TRUE(hook.Enable().has_value());
  ASSERT_TRUE(hook.Disable().has_value());

  ASSERT_TRUE(hook.Enable().has_value());

  EXPECT_EQ(CallSlot(), 2);
}
