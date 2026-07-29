#include <gtest/gtest.h>
#include <windows.h>

#include <afw/win/protect_guard.hpp>
#include <expected>
#include <support/win_probe.hpp>
#include <system_error>

namespace {

constexpr SIZE_T kProbeSize{4096};

class ProtectGuardUnit : public ::testing::Test {
 protected:
  void SetUp() override {
    page_ = VirtualAlloc(
        /*lpAddress        = */ nullptr,
        /*dwSize           = */ kProbeSize,
        /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
        /*flProtect        = */ PAGE_READONLY);
    ASSERT_NE(page_, nullptr);
    ASSERT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                               PAGE_READONLY));
  }

  void TearDown() override {
    if (page_ != nullptr) {
      VirtualFree(
          /*lpAddress  = */ page_,
          /*dwSize     = */ 0,
          /*dwFreeType = */ MEM_RELEASE);
    }
  }

  LPVOID page_{nullptr};
};

}  // namespace

TEST_F(ProtectGuardUnit, CreateRelaxesTheProtection) {
  const std::expected<afw::win::ProtectGuard, std::error_code> kGuard{
      afw::win::ProtectGuard::Create(GetCurrentProcess(), page_, kProbeSize,
                                     PAGE_READWRITE)};

  ASSERT_TRUE(kGuard.has_value()) << kGuard.error().message();
  EXPECT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                             PAGE_READWRITE));
}

TEST_F(ProtectGuardUnit, DestructorRestoresTheProtection) {
  {
    const std::expected<afw::win::ProtectGuard, std::error_code> kGuard{
        afw::win::ProtectGuard::Create(GetCurrentProcess(), page_, kProbeSize,
                                       PAGE_READWRITE)};
    ASSERT_TRUE(kGuard.has_value()) << kGuard.error().message();
  }

  EXPECT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                             PAGE_READONLY));
}

TEST_F(ProtectGuardUnit, RestoreReportsSuccessAndPutsTheProtectionBack) {
  std::expected<afw::win::ProtectGuard, std::error_code> guard{
      afw::win::ProtectGuard::Create(GetCurrentProcess(), page_, kProbeSize,
                                     PAGE_READWRITE)};
  ASSERT_TRUE(guard.has_value()) << guard.error().message();

  const std::expected<void, std::error_code> kRestored{guard->Restore()};

  EXPECT_TRUE(kRestored.has_value()) << kRestored.error().message();
  EXPECT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                             PAGE_READONLY));
}

TEST_F(ProtectGuardUnit, RestoreIsIdempotent) {
  std::expected<afw::win::ProtectGuard, std::error_code> guard{
      afw::win::ProtectGuard::Create(GetCurrentProcess(), page_, kProbeSize,
                                     PAGE_READWRITE)};
  ASSERT_TRUE(guard.has_value()) << guard.error().message();
  ASSERT_TRUE(guard->Restore().has_value());

  const DWORD kTightened{PAGE_NOACCESS};
  DWORD previous{};
  ASSERT_NE(VirtualProtect(
                /*lpAddress      = */ page_,
                /*dwSize         = */ kProbeSize,
                /*flNewProtect   = */ kTightened,
                /*lpflOldProtect = */ &previous),
            FALSE);

  EXPECT_TRUE(guard->Restore().has_value());
  EXPECT_TRUE(
      afw::test::RegionHasProtection(GetCurrentProcess(), page_, kTightened));
}

TEST_F(ProtectGuardUnit, AdoptRestoresOnDestruction) {
  DWORD previous{};
  ASSERT_NE(VirtualProtect(
                /*lpAddress      = */ page_,
                /*dwSize         = */ kProbeSize,
                /*flNewProtect   = */ PAGE_READWRITE,
                /*lpflOldProtect = */ &previous),
            FALSE);

  {
    const afw::win::ProtectGuard kGuard{afw::win::ProtectGuard::Adopt(
        GetCurrentProcess(), page_, kProbeSize, previous)};
    EXPECT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                               PAGE_READWRITE));
  }

  EXPECT_TRUE(afw::test::RegionHasProtection(GetCurrentProcess(), page_,
                                             PAGE_READONLY));
}

TEST_F(ProtectGuardUnit, CreateReportsTheWin32ErrorForAnUnmappedAddress) {
  const LPVOID kUnmapped{VirtualAlloc(
      /*lpAddress        = */ nullptr,
      /*dwSize           = */ kProbeSize,
      /*flAllocationType = */ MEM_RESERVE,
      /*flProtect        = */ PAGE_NOACCESS)};
  ASSERT_NE(kUnmapped, nullptr);
  ASSERT_NE(VirtualFree(
                /*lpAddress  = */ kUnmapped,
                /*dwSize     = */ 0,
                /*dwFreeType = */ MEM_RELEASE),
            FALSE);

  const std::expected<afw::win::ProtectGuard, std::error_code> kGuard{
      afw::win::ProtectGuard::Create(GetCurrentProcess(), kUnmapped, kProbeSize,
                                     PAGE_READWRITE)};

  ASSERT_FALSE(kGuard.has_value());
  EXPECT_EQ(&kGuard.error().category(), &std::system_category());
  EXPECT_NE(kGuard.error().value(), 0);
}
