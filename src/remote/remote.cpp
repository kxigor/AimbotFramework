// clang-format off
// Order pinned by hand: tlhelp32.h is not self-contained -- it has no #include
// of its own, yet its declarations use DWORD, WCHAR, MAX_PATH and WINAPI.
// Sorting it alphabetically puts it ahead of windows.h, its structs then fail
// to parse, and the errors surface far below as "no member named szExeFile".
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

using afw::win::LastError;
using afw::win::RemoteMemory;
using afw::win::ScopedHandle;

std::expected<DWORD, std::error_code> FindProcessByImageName(PCWSTR imageName) {
  const ScopedHandle kSnapshot{CreateToolhelp32Snapshot(
      /*dwFlags       = */ TH32CS_SNAPPROCESS,
      /*th32ProcessID = */ 0)};
  if (!kSnapshot) {
    return std::unexpected{LastError()};
  }

  PROCESSENTRY32W entry{.dwSize = sizeof(PROCESSENTRY32W)};

  BOOL hasEntry{Process32FirstW(
      /*hSnapshot = */ kSnapshot.Get(),
      /*lppe      = */ &entry)};
  if (hasEntry == FALSE) {
    return std::unexpected{LastError()};
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

std::expected<RemoteMemory, std::error_code> StrDupEx(HANDLE proc,
                                                      PCWSTR text) {
  const SIZE_T kByteSize{(wcslen(text) + 1) * sizeof(WCHAR)};

  LPVOID remoteText{VirtualAllocEx(
      /*hProcess         = */ proc,
      /*lpAddress        = */ nullptr,
      /*dwSize           = */ kByteSize,
      /*flAllocationType = */ MEM_COMMIT,
      /*flProtect        = */ PAGE_READWRITE)};
  if (remoteText == nullptr) {
    return std::unexpected{LastError()};
  }

  RemoteMemory remoteMem{proc, remoteText};

  const BOOL kWriteStatus{WriteProcessMemory(
      /*hProcess               = */ proc,
      /*lpBaseAddress          = */ remoteText,
      /*lpBuffer               = */ text,
      /*nSize                  = */ kByteSize,
      /*lpNumberOfBytesWritten = */ nullptr)};
  if (kWriteStatus == FALSE) {
    return std::unexpected{LastError()};
  }

  return remoteMem;
}

namespace {

[[nodiscard]] std::expected<LPTHREAD_START_ROUTINE, std::error_code>
LocalLoadLibraryRoutine() {
  HMODULE kernel32{GetModuleHandleW(
      /*lpModuleName = */ L"kernel32.dll")};
  if (kernel32 == nullptr) {
    return std::unexpected{LastError()};
  }

  FARPROC loadLibrary{GetProcAddress(
      /*hModule    = */ kernel32,
      /*lpProcName = */ "LoadLibraryW")};
  if (loadLibrary == nullptr) {
    return std::unexpected{LastError()};
  }

  // kernel32.dll is mapped at the same base in every process of a session, so
  // the local address of LoadLibraryW is also valid in the target process. Its
  // signature differs from LPTHREAD_START_ROUTINE, but the two are
  // ABI-compatible -- one pointer in, a pointer-sized value out -- and that is
  // what makes the CreateRemoteThread trick work. This one cast is spelled out
  // rather than routed through core::PointerCast: -Wcast-function-type fires in
  // the template's body, where a pragma at the call site cannot reach it, and
  // suppressing it inside the shared helper would silence it for every caller.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary);
#pragma GCC diagnostic pop
}

[[nodiscard]] std::expected<DWORD, std::error_code> RunRemoteRoutine(
    HANDLE proc, LPTHREAD_START_ROUTINE routine, RemoteMemory& argument,
    DWORD timeoutMs) {
  const ScopedHandle kThread{CreateRemoteThread(
      /*hProcess           = */ proc,
      /*lpThreadAttributes = */ nullptr,
      /*dwStackSize        = */ 0,
      /*lpStartAddress     = */ routine,
      /*lpParameter        = */ argument.Get(),
      /*dwCreationFlags    = */ 0,
      /*lpThreadId         = */ nullptr)};
  if (!kThread) {
    return std::unexpected{LastError()};
  }

  const DWORD kWaitResult{WaitForSingleObject(
      /*hHandle        = */ kThread.Get(),
      /*dwMilliseconds = */ timeoutMs)};
  if (kWaitResult != WAIT_OBJECT_0) {
    std::ignore = argument.Release();
    if (kWaitResult == WAIT_TIMEOUT) {
      return std::unexpected{make_error_code(Error::RemoteThreadTimeout)};
    }
    return std::unexpected{LastError()};
  }

  DWORD exitCode{};
  if (GetExitCodeThread(
          /*hThread    = */ kThread.Get(),
          /*lpExitCode = */ &exitCode) == FALSE) {
    return std::unexpected{LastError()};
  }

  return exitCode;
}

}  // namespace

std::expected<void, std::error_code> InjectLibrary(HANDLE proc, PCWSTR dllPath,
                                                   DWORD timeoutMs) {
  std::expected<RemoteMemory, std::error_code> remotePath{
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
