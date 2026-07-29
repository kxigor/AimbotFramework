#include <gtest/gtest.h>
#include <windows.h>

#include <afw/remote/error.hpp>
#include <afw/remote/remote.hpp>
#include <afw/win/remote_memory.hpp>
#include <afw/win/scoped_handle.hpp>
#include <array>
#include <cstddef>
#include <cwchar>
#include <expected>
#include <filesystem>
#include <string>
#include <support/win_probe.hpp>
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

// A copy of this test binary, created suspended and never resumed. It is a
// real foreign process: a different address space, reached only through the
// process handle. Terminated by the caller.
[[nodiscard]] PROCESS_INFORMATION SpawnSuspendedSelf() {
  std::array<WCHAR, MAX_PATH> exePath{};
  const DWORD kLength{GetModuleFileNameW(
      /*hModule    = */ nullptr,
      /*lpFilename = */ exePath.data(),
      /*nSize      = */ static_cast<DWORD>(MAX_PATH))};
  if (kLength == 0 || kLength >= static_cast<DWORD>(MAX_PATH)) {
    return {};
  }

  STARTUPINFOW startup{.cb = sizeof(STARTUPINFOW)};
  PROCESS_INFORMATION info{};
  if (CreateProcessW(
          /*lpApplicationName    = */ exePath.data(),
          /*lpCommandLine        = */ nullptr,
          /*lpProcessAttributes  = */ nullptr,
          /*lpThreadAttributes   = */ nullptr,
          /*bInheritHandles      = */ FALSE,
          /*dwCreationFlags      = */ CREATE_SUSPENDED,
          /*lpEnvironment        = */ nullptr,
          /*lpCurrentDirectory   = */ nullptr,
          /*lpStartupInfo        = */ &startup,
          /*lpProcessInformation = */ &info) == FALSE) {
    return {};
  }

  return info;
}

[[nodiscard]] afw::win::ScopedHandle OpenSelf() {
  return afw::win::ScopedHandle{OpenProcess(
      /*dwDesiredAccess = */ PROCESS_ALL_ACCESS,
      /*bInheritHandle  = */ FALSE,
      /*dwProcessId     = */ GetCurrentProcessId())};
}

}  // namespace

TEST(RemoteInjectUnit, MapsTheModuleIntoTheTargetProcess) {
  if (GetModuleHandleW(/*lpModuleName = */ kProbeDllName) != nullptr) {
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

  const HMODULE kLoaded{GetModuleHandleW(/*lpModuleName = */ kProbeDllName)};
  EXPECT_NE(kLoaded, nullptr);
  if (kLoaded != nullptr) {
    FreeLibrary(
        /*hLibModule = */ kLoaded);
  }
}

TEST(RemoteInjectUnit, ReportsFailureWhenTheRemoteLoadLibraryReturnsNull) {
  const std::wstring kMissingDll{L"C:\\afw-no-such-dll-9f3c1d.dll"};
  ASSERT_FALSE(std::filesystem::exists(kMissingDll));

  const afw::win::ScopedHandle kProc{OpenSelf()};
  ASSERT_TRUE(static_cast<bool>(kProc));

  const std::expected<void, std::error_code> kResult{
      afw::remote::InjectLibrary(kProc.Get(), kMissingDll.c_str())};

  ASSERT_FALSE(kResult.has_value());
  EXPECT_EQ(kResult.error(), afw::remote::make_error_code(
                                 afw::remote::Error::LoadLibraryFailed));
  EXPECT_EQ(&kResult.error().category(), &afw::remote::ErrorCategory());
}

TEST(RemoteInjectUnit, ReportsTheWin32ErrorWhenTheTargetHandleIsInvalid) {
  const std::wstring kDllPath{SystemDllPath(kProbeDllName)};
  ASSERT_FALSE(kDllPath.empty());

  const std::expected<void, std::error_code> kResult{
      afw::remote::InjectLibrary(nullptr, kDllPath.c_str())};

  ASSERT_FALSE(kResult.has_value());
  EXPECT_EQ(&kResult.error().category(), &std::system_category());
  EXPECT_NE(kResult.error().value(), 0);
}

TEST(RemoteInjectUnit, ReportsTimeoutWhenTheRemoteThreadOutlivesTheDeadline) {
  // A zero deadline makes WaitForSingleObject return WAIT_TIMEOUT unless the
  // thread is already signalled at the moment of the call. d3d11.dll is not
  // loaded in this binary and pulls a dependency chain, so the remote
  // LoadLibraryW cannot finish inside the microseconds before the wait starts.
  const std::wstring kSlowDll{SystemDllPath(L"d3d11.dll")};
  ASSERT_FALSE(kSlowDll.empty());
  ASSERT_EQ(GetModuleHandleW(/*lpModuleName = */ L"d3d11.dll"), nullptr);

  const afw::win::ScopedHandle kProc{OpenSelf()};
  ASSERT_TRUE(static_cast<bool>(kProc));

  const std::expected<void, std::error_code> kResult{
      afw::remote::InjectLibrary(kProc.Get(), kSlowDll.c_str(), 0)};

  ASSERT_FALSE(kResult.has_value());
  EXPECT_EQ(kResult.error(), afw::remote::make_error_code(
                                 afw::remote::Error::RemoteThreadTimeout));
}

TEST(RemoteInjectUnit, CopiesIntoAForeignProcess) {
  const PROCESS_INFORMATION kChild{SpawnSuspendedSelf()};
  ASSERT_NE(kChild.hProcess, nullptr);
  const afw::win::ScopedHandle kProc{kChild.hProcess};
  const afw::win::ScopedHandle kThread{kChild.hThread};

  constexpr PCWSTR kSource{L"afw cross-process probe"};
  const std::size_t kCharCount{std::wcslen(kSource) + 1};

  LPVOID observed{nullptr};
  {
    const std::expected<afw::win::RemoteMemory, std::error_code> kRemote{
        afw::remote::StrDupEx(kProc.Get(), kSource)};
    ASSERT_TRUE(kRemote.has_value()) << kRemote.error().message();

    observed = kRemote->Get();
    ASSERT_NE(observed, nullptr);
    EXPECT_TRUE(afw::test::RegionIsCommitted(kProc.Get(), observed));

    std::vector<WCHAR> readBack(kCharCount, L'\0');
    SIZE_T bytesRead{};
    ASSERT_NE(ReadProcessMemory(
                  /*hProcess            = */ kProc.Get(),
                  /*lpBaseAddress       = */ observed,
                  /*lpBuffer            = */ readBack.data(),
                  /*nSize               = */ kCharCount * sizeof(WCHAR),
                  /*lpNumberOfBytesRead = */ &bytesRead),
              FALSE);
    EXPECT_STREQ(readBack.data(), kSource);
  }

  EXPECT_TRUE(afw::test::RegionIsFree(kProc.Get(), observed));

  TerminateProcess(
      /*hProcess  = */ kProc.Get(),
      /*uExitCode = */ 0);
}

TEST(RemoteInjectUnit, CopiesTheNulTerminatedPathIntoTheTarget) {
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
    SIZE_T bytesRead{};
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
