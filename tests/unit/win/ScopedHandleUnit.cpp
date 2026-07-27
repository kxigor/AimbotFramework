// clang-format off
#include <windows.h>

#include <gtest/gtest.h>

#include <afw/win/scoped_handle.hpp>
#include <support/WinProbe.hpp>

#include <string>
#include <utility>

namespace {

[[nodiscard]] HANDLE CreateUnnamedEvent() {
  return CreateEventW(
      /*lpEventAttributes = */ nullptr,
      /*bManualReset      = */ TRUE,
      /*bInitialState     = */ FALSE,
      /*lpName            = */ nullptr);
}

}  // namespace

TEST(ScopedHandleUnit, DefaultConstructedOwnsNothing) {
  const afw::win::ScopedHandle kGuard{};

  EXPECT_EQ(kGuard.Get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(kGuard));
}

TEST(ScopedHandleUnit, InvalidHandleValueIsTreatedAsEmpty) {
  const afw::win::ScopedHandle kGuard{INVALID_HANDLE_VALUE};

  EXPECT_FALSE(static_cast<bool>(kGuard));
  EXPECT_EQ(kGuard.Get(), INVALID_HANDLE_VALUE);
}

TEST(ScopedHandleUnit, DestructorClosesTheHandle) {
  const std::wstring kName{afw::test::ProbeName(L"dtor")};
  const HANDLE kRaw{afw::test::CreateProbeEvent(kName)};
  ASSERT_NE(kRaw, nullptr);
  ASSERT_TRUE(afw::test::ProbeObjectExists(kName));

  {
    const afw::win::ScopedHandle kGuard{kRaw};
    EXPECT_TRUE(static_cast<bool>(kGuard));
    EXPECT_EQ(kGuard.Get(), kRaw);
  }

  EXPECT_FALSE(afw::test::ProbeObjectExists(kName));
}

TEST(ScopedHandleUnit, DestructorClosesTheHandleValue) {
  const HANDLE kRaw{CreateUnnamedEvent()};
  ASSERT_NE(kRaw, nullptr);

  {
    const afw::win::ScopedHandle kGuard{kRaw};
    EXPECT_TRUE(static_cast<bool>(kGuard));
  }

  EXPECT_TRUE(afw::test::HandleIsClosed(kRaw));
}

TEST(ScopedHandleUnit, ReleaseTransfersOwnershipAwayFromTheGuard) {
  const HANDLE kRaw{CreateUnnamedEvent()};
  ASSERT_NE(kRaw, nullptr);

  {
    afw::win::ScopedHandle guard{kRaw};

    EXPECT_EQ(guard.Release(), kRaw);
    EXPECT_FALSE(static_cast<bool>(guard));
    EXPECT_EQ(guard.Get(), nullptr);
  }

  EXPECT_FALSE(afw::test::HandleIsClosed(kRaw));
  EXPECT_NE(CloseHandle(
                /*hObject = */ kRaw),
            FALSE);
}

TEST(ScopedHandleUnit, ResetWithTheSameHandleIsANoOp) {
  const HANDLE kRaw{CreateUnnamedEvent()};
  ASSERT_NE(kRaw, nullptr);

  {
    afw::win::ScopedHandle guard{kRaw};
    guard.Reset(kRaw);

    EXPECT_FALSE(afw::test::HandleIsClosed(kRaw));
    EXPECT_EQ(guard.Get(), kRaw);
  }

  EXPECT_TRUE(afw::test::HandleIsClosed(kRaw));
}

TEST(ScopedHandleUnit, ResetClosesTheOldHandleAndAdoptsTheNew) {
  const HANDLE kFirst{CreateUnnamedEvent()};
  ASSERT_NE(kFirst, nullptr);
  const HANDLE kSecond{CreateUnnamedEvent()};
  ASSERT_NE(kSecond, nullptr);
  ASSERT_NE(kFirst, kSecond);

  {
    afw::win::ScopedHandle guard{kFirst};
    guard.Reset(kSecond);

    EXPECT_TRUE(afw::test::HandleIsClosed(kFirst));
    EXPECT_EQ(guard.Get(), kSecond);
  }

  EXPECT_TRUE(afw::test::HandleIsClosed(kSecond));
}

TEST(ScopedHandleUnit, MoveConstructionTransfersOwnership) {
  const HANDLE kRaw{CreateUnnamedEvent()};
  ASSERT_NE(kRaw, nullptr);

  {
    afw::win::ScopedHandle src{kRaw};
    const afw::win::ScopedHandle kDst{std::move(src)};

    EXPECT_EQ(kDst.Get(), kRaw);
    EXPECT_FALSE(static_cast<bool>(src));
    EXPECT_EQ(src.Get(), nullptr);
    EXPECT_FALSE(afw::test::HandleIsClosed(kRaw));
  }

  EXPECT_TRUE(afw::test::HandleIsClosed(kRaw));
}

TEST(ScopedHandleUnit, MoveAssignmentClosesTheHandleItReplaces) {
  const std::wstring kVictimName{afw::test::ProbeName(L"moveassign")};
  const HANDLE kVictim{afw::test::CreateProbeEvent(kVictimName)};
  ASSERT_NE(kVictim, nullptr);
  const HANDLE kSurvivor{CreateUnnamedEvent()};
  ASSERT_NE(kSurvivor, nullptr);

  {
    afw::win::ScopedHandle dst{kVictim};
    afw::win::ScopedHandle src{kSurvivor};
    dst = std::move(src);

    EXPECT_FALSE(afw::test::ProbeObjectExists(kVictimName));
    EXPECT_EQ(dst.Get(), kSurvivor);
    EXPECT_FALSE(static_cast<bool>(src));
    EXPECT_EQ(src.Get(), nullptr);
  }

  EXPECT_TRUE(afw::test::HandleIsClosed(kSurvivor));
}

TEST(ScopedHandleUnit, SelfMoveAssignmentKeepsTheHandle) {
  const HANDLE kRaw{CreateUnnamedEvent()};
  ASSERT_NE(kRaw, nullptr);

  {
    afw::win::ScopedHandle guard{kRaw};

    guard = std::move(guard);

    EXPECT_TRUE(static_cast<bool>(guard));
    EXPECT_EQ(guard.Get(), kRaw);
    EXPECT_FALSE(afw::test::HandleIsClosed(kRaw));
  }

  EXPECT_TRUE(afw::test::HandleIsClosed(kRaw));
}
