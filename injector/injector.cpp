// clang-format off
#include <windows.h>

#include <afw/remote/remote.hpp>
#include <afw/win/scoped_handle.hpp>

#include <cstdio>
#include <cwchar>
#include <expected>
#include <filesystem>
#include <system_error>

namespace {

constexpr PCWSTR kProcName = L"notepad.exe";
constexpr PCWSTR kDllName = L"aimbot.dll";

// Absolute path to the DLL that is staged next to this executable.
std::filesystem::path ResolveDllPath() {
  WCHAR exePath[MAX_PATH]{};
  DWORD length{GetModuleFileNameW(
      /*hModule    = */ nullptr,
      /*lpFilename = */ &exePath[0],
      /*nSize      = */ static_cast<DWORD>(MAX_PATH))};
  if (length == 0 || length >= static_cast<DWORD>(MAX_PATH)) {
    return {};
  }
  return std::filesystem::path(&exePath[0]).parent_path() / kDllName;
}

}  // namespace

int main() {
  std::expected<DWORD, std::error_code> pid{
      afw::remote::GetPidByProcName(kProcName)};
  if (!pid) {
    std::fwprintf(stderr, L"[injector] target process not found: %ls (%hs)\n",
                  kProcName, pid.error().message().c_str());
    return 1;
  }

  afw::win::ScopedHandle proc{OpenProcess(
      /*dwDesiredAccess = */ PROCESS_ALL_ACCESS,
      /*bInheritHandle  = */ FALSE,
      /*dwProcessId     = */ *pid)};
  if (!proc) {
    std::fwprintf(stderr, L"[injector] OpenProcess failed (error %lu)\n",
                  GetLastError());
    return 1;
  }

  std::filesystem::path dllPath{ResolveDllPath()};
  if (dllPath.empty()) {
    std::fwprintf(stderr, L"[injector] could not resolve the DLL path\n");
    return 1;
  }

  std::expected<void, std::error_code> injected{
      afw::remote::LoadDll(proc.Get(), dllPath.c_str())};
  if (!injected) {
    std::fwprintf(stderr, L"[injector] injection failed: %hs\n",
                  injected.error().message().c_str());
    return 1;
  }

  std::wprintf(L"[injector] injected \"%ls\" into %ls (pid %lu)\n",
               dllPath.c_str(), kProcName, *pid);
  return 0;
}
