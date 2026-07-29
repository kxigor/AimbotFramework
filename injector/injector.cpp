#include <windows.h>

#include <afw/remote/remote.hpp>
#include <afw/win/scoped_handle.hpp>
#include <array>
#include <cstdio>
#include <cwchar>
#include <expected>
#include <filesystem>
#include <system_error>

namespace {

constexpr PCWSTR kTargetImageName{L"notepad.exe"};
constexpr PCWSTR kDllName{L"aimbot.dll"};

// Absolute path to the DLL that is staged next to this executable.
[[nodiscard]] std::filesystem::path ResolveDllPath() {
  std::array<WCHAR, MAX_PATH> exePath{};
  const DWORD kLength{GetModuleFileNameW(
      /*hModule    = */ nullptr,
      /*lpFilename = */ exePath.data(),
      /*nSize      = */ static_cast<DWORD>(MAX_PATH))};
  if (kLength == 0 || kLength >= static_cast<DWORD>(MAX_PATH)) {
    return {};
  }
  return std::filesystem::path{exePath.data()}.parent_path() / kDllName;
}

}  // namespace

int main() {
  const std::expected<DWORD, std::error_code> kPid{
      afw::remote::FindProcessByImageName(kTargetImageName)};
  if (!kPid) {
    std::fwprintf(stderr, L"[injector] target process not found: %ls (%hs)\n",
                  kTargetImageName, kPid.error().message().c_str());
    return 1;
  }

  const afw::win::ScopedHandle kProc{OpenProcess(
      /*dwDesiredAccess = */ PROCESS_ALL_ACCESS,
      /*bInheritHandle  = */ FALSE,
      /*dwProcessId     = */ *kPid)};
  if (!kProc) {
    std::fwprintf(stderr, L"[injector] OpenProcess failed (error %lu)\n",
                  GetLastError());
    return 1;
  }

  const std::filesystem::path kDllPath{ResolveDllPath()};
  if (kDllPath.empty()) {
    std::fwprintf(stderr, L"[injector] could not resolve the DLL path\n");
    return 1;
  }

  const std::expected<void, std::error_code> kInjected{
      afw::remote::InjectLibrary(kProc.Get(), kDllPath.c_str())};
  if (!kInjected) {
    std::fwprintf(stderr, L"[injector] injection failed: %hs\n",
                  kInjected.error().message().c_str());
    return 1;
  }

  std::wprintf(L"[injector] injected \"%ls\" into %ls (pid %lu)\n",
               kDllPath.c_str(), kTargetImageName, *kPid);
  return 0;
}
