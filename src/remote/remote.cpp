// clang-format off
#include <windows.h>

#include <tlhelp32.h>
// clang-format on

#include <afw/remote/error.hpp>
#include <afw/remote/remote.hpp>
#include <afw/win/error.hpp>
#include <afw/win/remote_memory.hpp>
#include <afw/win/scoped_handle.hpp>
#include <cwchar>
#include <expected>
#include <system_error>
#include <tuple>

namespace afw::remote {

std::expected<DWORD, std::error_code> FindProcessByImageName(PCWSTR imageName) {
  const win::ScopedHandle kSnapshot{CreateToolhelp32Snapshot(
      /*dwFlags       = */ TH32CS_SNAPPROCESS,
      /*th32ProcessID = */ 0)};
  if (!kSnapshot) {
    return std::unexpected{win::LastError()};
  }

  PROCESSENTRY32W entry{.dwSize = sizeof(PROCESSENTRY32W)};

  BOOL hasEntry{Process32FirstW(
      /*hSnapshot = */ kSnapshot.Get(),
      /*lppe      = */ &entry)};
  if (hasEntry == FALSE) {
    return std::unexpected{win::LastError()};
  }

  while (hasEntry != FALSE) {
    if (_wcsicmp(&entry.szExeFile[0], imageName) == 0) {
      return entry.th32ProcessID;
    }
    hasEntry = Process32NextW(
        /*hSnapshot = */ kSnapshot.Get(),
        /*lppe      = */ &entry);
  }

  return std::unexpected{std::make_error_code(std::errc::no_such_process)};
}

std::expected<win::RemoteMemory, std::error_code> StrDupEx(HANDLE proc,
                                                           PCWSTR text) {
  const SIZE_T kByteSize{(wcslen(text) + 1) * sizeof(WCHAR)};

  LPVOID remoteText{VirtualAllocEx(
      /*hProcess         = */ proc,
      /*lpAddress        = */ nullptr,
      /*dwSize           = */ kByteSize,
      /*flAllocationType = */ MEM_COMMIT,
      /*flProtect        = */ PAGE_READWRITE)};
  if (remoteText == nullptr) {
    return std::unexpected{win::LastError()};
  }

  win::RemoteMemory remoteMem{proc, remoteText};

  const BOOL kWriteStatus{WriteProcessMemory(
      /*hProcess               = */ proc,
      /*lpBaseAddress          = */ remoteText,
      /*lpBuffer               = */ text,
      /*nSize                  = */ kByteSize,
      /*lpNumberOfBytesWritten = */ nullptr)};
  if (kWriteStatus == FALSE) {
    return std::unexpected{win::LastError()};
  }

  return remoteMem;
}

namespace {

[[nodiscard]] std::expected<LPTHREAD_START_ROUTINE, std::error_code>
LocalLoadLibraryRoutine() {
  HMODULE kernel32{GetModuleHandleW(
      /*lpModuleName = */ L"kernel32.dll")};
  if (kernel32 == nullptr) {
    return std::unexpected{win::LastError()};
  }

  FARPROC loadLibrary{GetProcAddress(
      /*hModule    = */ kernel32,
      /*lpProcName = */ "LoadLibraryW")};
  if (loadLibrary == nullptr) {
    return std::unexpected{win::LastError()};
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary);
#pragma GCC diagnostic pop
}

[[nodiscard]] std::expected<DWORD, std::error_code> RunRemoteRoutine(
    HANDLE proc, LPTHREAD_START_ROUTINE routine, win::RemoteMemory& argument,
    DWORD timeoutMs) {
  const win::ScopedHandle kThread{CreateRemoteThread(
      /*hProcess           = */ proc,
      /*lpThreadAttributes = */ nullptr,
      /*dwStackSize        = */ 0,
      /*lpStartAddress     = */ routine,
      /*lpParameter        = */ argument.Get(),
      /*dwCreationFlags    = */ 0,
      /*lpThreadId         = */ nullptr)};
  if (!kThread) {
    return std::unexpected{win::LastError()};
  }

  const DWORD kWaitResult{WaitForSingleObject(
      /*hHandle        = */ kThread.Get(),
      /*dwMilliseconds = */ timeoutMs)};
  if (kWaitResult != WAIT_OBJECT_0) {
    std::ignore = argument.Release();
    if (kWaitResult == WAIT_TIMEOUT) {
      return std::unexpected{make_error_code(Error::RemoteThreadTimeout)};
    }
    return std::unexpected{win::LastError()};
  }

  DWORD exitCode{};
  if (GetExitCodeThread(
          /*hThread    = */ kThread.Get(),
          /*lpExitCode = */ &exitCode) == FALSE) {
    return std::unexpected{win::LastError()};
  }

  return exitCode;
}

}  // namespace

std::expected<void, std::error_code> InjectLibrary(HANDLE proc, PCWSTR dllPath,
                                                   DWORD timeoutMs) {
  std::expected<win::RemoteMemory, std::error_code> remotePath{
      StrDupEx(proc, dllPath)};
  if (!remotePath) {
    return std::unexpected{remotePath.error()};
  }

  const std::expected<LPTHREAD_START_ROUTINE, std::error_code> kLoadLibrary{
      LocalLoadLibraryRoutine()};
  if (!kLoadLibrary) {
    return std::unexpected{kLoadLibrary.error()};
  }

  const std::expected<DWORD, std::error_code> kExitCode{
      RunRemoteRoutine(proc, *kLoadLibrary, *remotePath, timeoutMs)};
  if (!kExitCode) {
    return std::unexpected{kExitCode.error()};
  }
  if (*kExitCode == 0) {
    return std::unexpected{make_error_code(Error::LoadLibraryFailed)};
  }

  return {};
}

}  // namespace afw::remote
