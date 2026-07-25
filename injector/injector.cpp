// clang-format off
#include <windows.h>

#include <afw/remote/remote.hpp>
#include <afw/win/scoped_handle.hpp>

#include <array>
#include <cstdio>
#include <cwchar>
#include <expected>
#include <filesystem>
#include <system_error>

constexpr PCWSTR kProcName = L"notepad.exe";
constexpr PCWSTR kDllName = L"aimbot.dll";

// Absolute path to the DLL that is staged next to this executable.
static std::filesystem::path ResolveDllPath() {
  std::array<WCHAR, MAX_PATH> exePath{};
  const DWORD kLength{GetModuleFileNameW(
      /*hModule    = */ nullptr,
      /*lpFilename = */ exePath.data(),
      /*nSize      = */ static_cast<DWORD>(MAX_PATH))};
  if (kLength == 0 || kLength >= static_cast<DWORD>(MAX_PATH)) {
    return {};
  }
  return std::filesystem::path(exePath.data()).parent_path() / kDllName;
}

int main() {
  std::expected<DWORD, std::error_code> pid{
      afw::remote::GetPidByProcName(kProcName)};
  if (!pid) {
    std::fwprintf(stderr, L"[injector] target process not found: %ls (%hs)\n",
                  kProcName, pid.error().message().c_str());
    return 1;
  }

  const afw::win::ScopedHandle kProc{OpenProcess(
      /*dwDesiredAccess = */ PROCESS_ALL_ACCESS,
      /*bInheritHandle  = */ FALSE,
      /*dwProcessId     = */ *pid)};
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

  std::expected<void, std::error_code> injected{
      afw::remote::LoadDll(kProc.Get(), kDllPath.c_str())};
  if (!injected) {
    std::fwprintf(stderr, L"[injector] injection failed: %hs\n",
                  injected.error().message().c_str());
    return 1;
  }

  std::wprintf(L"[injector] injected \"%ls\" into %ls (pid %lu)\n",
               kDllPath.c_str(), kProcName, *pid);
  return 0;
}
