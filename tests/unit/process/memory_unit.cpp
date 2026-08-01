#include <gtest/gtest.h>
#include <windows.h>

#include <afw/process/error.hpp>
#include <afw/process/memory.hpp>
#include <bit>
#include <cstdint>
#include <expected>
#include <support/win_probe.hpp>
#include <system_error>

namespace {

constexpr SIZE_T kProbeSize{4096};
constexpr std::uint32_t kSentinel{0xA1B2C3D4};

}  // namespace

TEST(ProcessMemoryUnit, ModuleBaseFindsALoadedModule) {
  const std::expected<std::uintptr_t, std::error_code> kBase{
      afw::process::ModuleBase(L"kernel32.dll")};

  ASSERT_TRUE(kBase.has_value()) << kBase.error().message();
  EXPECT_EQ(*kBase, std::bit_cast<std::uintptr_t>(
                        GetModuleHandleW(/*lpModuleName = */ L"kernel32.dll")));
}

TEST(ProcessMemoryUnit, ModuleBaseReportsAModuleThatIsNotLoaded) {
  const std::expected<std::uintptr_t, std::error_code> kBase{
      afw::process::ModuleBase(L"afw_no_such_module.dll")};

  ASSERT_FALSE(kBase.has_value());
  EXPECT_EQ(kBase.error(),
            afw::process::make_error_code(afw::process::Error::ModuleNotFound));
  EXPECT_EQ(&kBase.error().category(), &afw::process::ErrorCategory());
}

TEST(ProcessMemoryUnit, ReadAtReturnsWhatIsThere) {
  const std::uint32_t kValue{kSentinel};

  const std::expected<std::uint32_t, std::error_code> kRead{
      afw::process::ReadAt<std::uint32_t>(
          std::bit_cast<std::uintptr_t>(static_cast<const void*>(&kValue)))};

  ASSERT_TRUE(kRead.has_value()) << kRead.error().message();
  EXPECT_EQ(*kRead, kSentinel);
}

TEST(ProcessMemoryUnit, ReadAtRejectsANullAddress) {
  const std::expected<std::uint32_t, std::error_code> kRead{
      afw::process::ReadAt<std::uint32_t>(0)};

  ASSERT_FALSE(kRead.has_value());
  EXPECT_EQ(kRead.error(),
            afw::process::make_error_code(afw::process::Error::NotReadable));
}

TEST(ProcessMemoryUnit, ReadAtRejectsAFreedRegion) {
  const LPVOID kPage{VirtualAlloc(
      /*lpAddress        = */ nullptr,
      /*dwSize           = */ kProbeSize,
      /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
      /*flProtect        = */ PAGE_READWRITE)};
  ASSERT_NE(kPage, nullptr);
  ASSERT_NE(VirtualFree(
                /*lpAddress  = */ kPage,
                /*dwSize     = */ 0,
                /*dwFreeType = */ MEM_RELEASE),
            FALSE);
  ASSERT_TRUE(afw::test::RegionIsFree(GetCurrentProcess(), kPage));

  const std::expected<std::uint32_t, std::error_code> kRead{
      afw::process::ReadAt<std::uint32_t>(
          std::bit_cast<std::uintptr_t>(kPage))};

  ASSERT_FALSE(kRead.has_value());
  EXPECT_EQ(kRead.error(),
            afw::process::make_error_code(afw::process::Error::NotReadable));
}

TEST(ProcessMemoryUnit, ReadAtRejectsAPageThatDeniesAccess) {
  const LPVOID kPage{VirtualAlloc(
      /*lpAddress        = */ nullptr,
      /*dwSize           = */ kProbeSize,
      /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
      /*flProtect        = */ PAGE_NOACCESS)};
  ASSERT_NE(kPage, nullptr);

  const std::expected<std::uint32_t, std::error_code> kRead{
      afw::process::ReadAt<std::uint32_t>(
          std::bit_cast<std::uintptr_t>(kPage))};

  EXPECT_FALSE(kRead.has_value());

  VirtualFree(
      /*lpAddress  = */ kPage,
      /*dwSize     = */ 0,
      /*dwFreeType = */ MEM_RELEASE);
}

TEST(ProcessMemoryUnit, ReadAtRejectsAReadThatWouldLeaveTheRegion) {
  const LPVOID kPage{VirtualAlloc(
      /*lpAddress        = */ nullptr,
      /*dwSize           = */ kProbeSize,
      /*flAllocationType = */ MEM_COMMIT | MEM_RESERVE,
      /*flProtect        = */ PAGE_READWRITE)};
  ASSERT_NE(kPage, nullptr);

  const std::uintptr_t kLastByte{std::bit_cast<std::uintptr_t>(kPage) +
                                 kProbeSize - 1};

  EXPECT_TRUE(afw::process::ReadAt<std::uint8_t>(kLastByte).has_value());
  EXPECT_FALSE(afw::process::ReadAt<std::uint32_t>(kLastByte).has_value());

  VirtualFree(
      /*lpAddress  = */ kPage,
      /*dwSize     = */ 0,
      /*dwFreeType = */ MEM_RELEASE);
}
