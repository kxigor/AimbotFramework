// clang-format off
#include <windows.h>

#include <gtest/gtest.h>

#include <afw/win/remote_memory.hpp>
#include <support/WinProbe.hpp>

#include <utility>

namespace {

constexpr SIZE_T kProbeSize{4096};

class RemoteMemoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    proc_ = OpenProcess(
        /*dwDesiredAccess = */ PROCESS_ALL_ACCESS,
        /*bInheritHandle  = */ FALSE,
        /*dwProcessId     = */ GetCurrentProcessId());
    ASSERT_NE(proc_, nullptr);
  }

  void TearDown() override {
    if (proc_ != nullptr) {
      CloseHandle(
          /*hObject = */ proc_);
    }
  }

  [[nodiscard]] LPVOID Allocate() const {
    return VirtualAllocEx(
        /*hProcess         = */ proc_,
        /*lpAddress        = */ nullptr,
        /*dwSize           = */ kProbeSize,
        /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
        /*flProtect        = */ PAGE_READWRITE);
  }

  HANDLE proc_{nullptr};
};

}  // namespace

TEST_F(RemoteMemoryTest, DefaultConstructedOwnsNothing) {
  const afw::win::RemoteMemory kMem{};

  EXPECT_EQ(kMem.Get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(kMem));
}

TEST_F(RemoteMemoryTest, DestructorReleasesTheAllocation) {
  const LPVOID kAddr{Allocate()};
  ASSERT_NE(kAddr, nullptr);

  {
    const afw::win::RemoteMemory kMem{proc_, kAddr};
    EXPECT_TRUE(static_cast<bool>(kMem));
    EXPECT_EQ(kMem.Get(), kAddr);
    EXPECT_TRUE(afw::test::RegionIsCommitted(proc_, kAddr));
  }

  EXPECT_TRUE(afw::test::RegionIsFree(proc_, kAddr));
}

TEST_F(RemoteMemoryTest, ReleaseKeepsTheAllocationAlive) {
  const LPVOID kAddr{Allocate()};
  ASSERT_NE(kAddr, nullptr);

  {
    afw::win::RemoteMemory mem{proc_, kAddr};

    EXPECT_EQ(mem.Release(), kAddr);
    EXPECT_FALSE(static_cast<bool>(mem));
    EXPECT_EQ(mem.Get(), nullptr);
  }

  EXPECT_TRUE(afw::test::RegionIsCommitted(proc_, kAddr));
  EXPECT_NE(VirtualFreeEx(
                /*hProcess   = */ proc_,
                /*lpAddress  = */ kAddr,
                /*dwSize     = */ 0,
                /*dwFreeType = */ MEM_RELEASE),
            FALSE);
}

TEST_F(RemoteMemoryTest, MoveConstructionTransfersOwnership) {
  const LPVOID kAddr{Allocate()};
  ASSERT_NE(kAddr, nullptr);

  {
    afw::win::RemoteMemory src{proc_, kAddr};
    const afw::win::RemoteMemory kDst{std::move(src)};

    EXPECT_EQ(kDst.Get(), kAddr);
    EXPECT_FALSE(static_cast<bool>(src));
    EXPECT_EQ(src.Get(), nullptr);
    EXPECT_TRUE(afw::test::RegionIsCommitted(proc_, kAddr));
  }

  EXPECT_TRUE(afw::test::RegionIsFree(proc_, kAddr));
}

TEST_F(RemoteMemoryTest, MoveAssignmentReleasesTheReplacedAllocation) {
  const LPVOID kOld{Allocate()};
  ASSERT_NE(kOld, nullptr);
  const LPVOID kNew{Allocate()};
  ASSERT_NE(kNew, nullptr);
  ASSERT_NE(kOld, kNew);

  {
    afw::win::RemoteMemory dst{proc_, kOld};
    afw::win::RemoteMemory src{proc_, kNew};
    dst = std::move(src);

    EXPECT_TRUE(afw::test::RegionIsFree(proc_, kOld));
    EXPECT_TRUE(afw::test::RegionIsCommitted(proc_, kNew));
    EXPECT_EQ(dst.Get(), kNew);
    EXPECT_FALSE(static_cast<bool>(src));
    EXPECT_EQ(src.Get(), nullptr);
  }

  EXPECT_TRUE(afw::test::RegionIsFree(proc_, kNew));
}

TEST_F(RemoteMemoryTest, ResetWithTheSameAddressDoesNotFree) {
  const LPVOID kAddr{Allocate()};
  ASSERT_NE(kAddr, nullptr);

  {
    afw::win::RemoteMemory mem{proc_, kAddr};
    mem.Reset(proc_, kAddr);

    EXPECT_TRUE(afw::test::RegionIsCommitted(proc_, kAddr));
    EXPECT_EQ(mem.Get(), kAddr);
  }

  EXPECT_TRUE(afw::test::RegionIsFree(proc_, kAddr));
}

TEST_F(RemoteMemoryTest, ResetReleasesTheOldAllocation) {
  const LPVOID kOld{Allocate()};
  ASSERT_NE(kOld, nullptr);
  const LPVOID kNew{Allocate()};
  ASSERT_NE(kNew, nullptr);
  ASSERT_NE(kOld, kNew);

  {
    afw::win::RemoteMemory mem{proc_, kOld};
    mem.Reset(proc_, kNew);

    EXPECT_TRUE(afw::test::RegionIsFree(proc_, kOld));
    EXPECT_TRUE(afw::test::RegionIsCommitted(proc_, kNew));
    EXPECT_EQ(mem.Get(), kNew);
  }

  EXPECT_TRUE(afw::test::RegionIsFree(proc_, kNew));
}
