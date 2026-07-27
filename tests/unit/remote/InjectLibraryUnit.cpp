// clang-format off
#include <windows.h>

#include <gtest/gtest.h>

#include <afw/remote/error.hpp>
#include <afw/remote/remote.hpp>
#include <afw/win/remote_memory.hpp>
#include <afw/win/scoped_handle.hpp>
#include <support/WinProbe.hpp>

#include <array>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace {

constexpr PCWSTR kProbeDllName{L"winmm.dll"};

[[nodiscard]] std::wstring SystemDllPath(PCWSTR name) {
  std::array<WCHAR, MAX_PATH> directory{};
  const UINT kLength{GetSystemDirectoryW(
      /*lpBuffer = */ directory.data(),
      /*uSize    = */ static_cast<UINT>(MAX_PATH))};
  if (kLength == 0 || kLength >= static_cast<UINT>(MAX_PATH)) {
    return {};
  }
  return (std::filesystem::path{directory.data()} / name).wstring();
}

[[nodiscard]] afw::win::ScopedHandle OpenSelf() {
  return afw::win::ScopedHandle{OpenProcess(
      /*dwDesiredAccess = */ PROCESS_ALL_ACCESS,
      /*bInheritHandle  = */ FALSE,
      /*dwProcessId     = */ GetCurrentProcessId())};
}

}  // namespace

TEST(InjectLibraryUnit, MapsTheModuleIntoTheTargetProcess) {
  if (GetModuleHandleW(kProbeDllName) != nullptr) {
    GTEST_SKIP() << "winmm.dll is already mapped into this process, so the "
                    "before/after module check cannot prove anything.";
  }

  const std::wstring kDllPath{SystemDllPath(kProbeDllName)};
  ASSERT_FALSE(kDllPath.empty());
  ASSERT_TRUE(std::filesystem::exists(kDllPath));

  const afw::win::ScopedHandle kProc{OpenSelf()};
  ASSERT_TRUE(static_cast<bool>(kProc));

  const std::expected<void, std::error_code> kResult{
      afw::remote::InjectLibrary(kProc.Get(), kDllPath.c_str())};
  ASSERT_TRUE(kResult.has_value()) << kResult.error().message();

  const HMODULE kLoaded{GetModuleHandleW(kProbeDllName)};
  EXPECT_NE(kLoaded, nullptr);
  if (kLoaded != nullptr) {
    FreeLibrary(
        /*hLibModule = */ kLoaded);
  }
}

TEST(InjectLibraryUnit, ReportsFailureWhenTheRemoteLoadLibraryReturnsNull) {
  const std::wstring kMissingDll{L"C:\\afw-no-such-dll-9f3c1d.dll"};
  ASSERT_FALSE(std::filesystem::exists(kMissingDll));

  const afw::win::ScopedHandle kProc{OpenSelf()};
  ASSERT_TRUE(static_cast<bool>(kProc));

  const std::expected<void, std::error_code> kResult{
      afw::remote::InjectLibrary(kProc.Get(), kMissingDll.c_str())};

  ASSERT_FALSE(kResult.has_value());
  EXPECT_EQ(kResult.error(),
            afw::remote::make_error_code(afw::remote::Error::LoadLibraryFailed));
  EXPECT_EQ(&kResult.error().category(), &afw::remote::ErrorCategory());
}

TEST(InjectLibraryUnit, ReportsTheWin32ErrorWhenTheTargetHandleIsInvalid) {
  const std::wstring kDllPath{SystemDllPath(kProbeDllName)};
  ASSERT_FALSE(kDllPath.empty());

  const std::expected<void, std::error_code> kResult{
      afw::remote::InjectLibrary(nullptr, kDllPath.c_str())};

  ASSERT_FALSE(kResult.has_value());
  EXPECT_EQ(&kResult.error().category(), &std::system_category());
  EXPECT_NE(kResult.error().value(), 0);
}

TEST(StrDupExUnit, CopiesTheNulTerminatedPathIntoTheTarget) {
  const std::wstring kSource{SystemDllPath(kProbeDllName)};
  ASSERT_FALSE(kSource.empty());
  const std::size_t kCharCount{kSource.size() + 1};

  const afw::win::ScopedHandle kProc{OpenSelf()};
  ASSERT_TRUE(static_cast<bool>(kProc));

  LPVOID observed{nullptr};
  {
    const std::expected<afw::win::RemoteMemory, std::error_code> kRemote{
        afw::remote::StrDupEx(kProc.Get(), kSource.c_str())};
    ASSERT_TRUE(kRemote.has_value()) << kRemote.error().message();

    observed = kRemote->Get();
    ASSERT_NE(observed, nullptr);
    EXPECT_TRUE(afw::test::RegionIsCommitted(kProc.Get(), observed));

    std::vector<WCHAR> readBack(kCharCount, L'\0');
    SIZE_T bytesRead{0};
    ASSERT_NE(ReadProcessMemory(
                  /*hProcess            = */ kProc.Get(),
                  /*lpBaseAddress       = */ observed,
                  /*lpBuffer            = */ readBack.data(),
                  /*nSize               = */ kCharCount * sizeof(WCHAR),
                  /*lpNumberOfBytesRead = */ &bytesRead),
              FALSE);
    EXPECT_EQ(bytesRead, kCharCount * sizeof(WCHAR));
    EXPECT_EQ(readBack[kCharCount - 1], L'\0');
    EXPECT_STREQ(readBack.data(), kSource.c_str());
  }

  EXPECT_TRUE(afw::test::RegionIsFree(kProc.Get(), observed));
}
