// clang-format off
#include <windows.h>

#include <tlhelp32.h>

#include <afw/remote/remote.hpp>
#include <afw/win/error.hpp>
#include <afw/win/remote_memory.hpp>
#include <afw/win/scoped_handle.hpp>

#include <cstddef>
#include <cwchar>
#include <expected>
#include <system_error>

namespace afw::remote {

using afw::win::LastError;
using afw::win::RemoteMemory;
using afw::win::ScopedHandle;

std::expected<DWORD, std::error_code> GetPidByProcName(PCWSTR procName) {
  const ScopedHandle kSnapshot{CreateToolhelp32Snapshot(
      /*dwFlags       = */ TH32CS_SNAPPROCESS,
      /*th32ProcessID = */ 0)};
  if (!kSnapshot) {
    return std::unexpected{LastError()};
  }

  PROCESSENTRY32W entry{.dwSize = sizeof(PROCESSENTRY32W)};

  BOOL hasProcess{Process32FirstW(
      /*hSnapshot = */ kSnapshot.Get(),
      /*lppe      = */ &entry)};
  if (hasProcess == FALSE) {
    return std::unexpected{LastError()};
  }

  while (hasProcess != FALSE) {
    if (_wcsicmp(&entry.szExeFile[0], procName) == 0) {
      return entry.th32ProcessID;
    }
    hasProcess = Process32NextW(
        /*hSnapshot = */ kSnapshot.Get(),
        /*lppe      = */ &entry);
  }

  return std::unexpected{std::make_error_code(std::errc::no_such_process)};
}

std::expected<RemoteMemory, std::error_code> StrDupEx(HANDLE proc,
                                                      PCWSTR dllPath) {
  const std::size_t kDllPathByteSize{(wcslen(dllPath) + 1) * sizeof(WCHAR)};

  LPVOID remoteDllPath{VirtualAllocEx(
      /*hProcess         = */ proc,
      /*lpAddress        = */ nullptr,
      /*dwSize           = */ kDllPathByteSize,
      /*flAllocationType = */ MEM_COMMIT,
      /*flProtect        = */ PAGE_READWRITE)};
  if (remoteDllPath == nullptr) {
    return std::unexpected{LastError()};
  }

  RemoteMemory remoteMem{proc, remoteDllPath};

  const BOOL kWriteStatus{WriteProcessMemory(
      /*hProcess               = */ proc,
      /*lpBaseAddress          = */ remoteDllPath,
      /*lpBuffer               = */ dllPath,
      /*nSize                  = */ kDllPathByteSize,
      /*lpNumberOfBytesWritten = */ nullptr)};
  if (kWriteStatus == FALSE) {
    return std::unexpected{LastError()};
  }

  return remoteMem;
}

std::expected<LPTHREAD_START_ROUTINE, std::error_code>
GetRemoteLoadLibraryAddr() {
  HMODULE k32{GetModuleHandleA(
      /*lpModuleName = */ "kernel32.dll")};
  if (k32 == nullptr) {
    return std::unexpected{LastError()};
  }

  FARPROC loadLibraryAddr{GetProcAddress(
      /*hModule    = */ k32,
      /*lpProcName = */ "LoadLibraryW")};
  if (loadLibraryAddr == nullptr) {
    return std::unexpected{LastError()};
  }

  // kernel32.dll is mapped at the same base in every process of a session, so
  // the local address of LoadLibraryW is also valid in the target process.
  // LoadLibraryW's signature differs from LPTHREAD_START_ROUTINE, but on x64
  // the two are ABI-compatible (one pointer in, pointer-sized value out) -- that
  // is what makes the CreateRemoteThread trick work. The type-mismatch cast is
  // therefore intentional; silence -Wcast-function-type just for it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr);
#pragma GCC diagnostic pop
}

std::expected<void, std::error_code> LoadDll(HANDLE proc, PCWSTR dllPath) {
  std::expected<RemoteMemory, std::error_code> remoteDllPath{
      StrDupEx(proc, dllPath)};
  if (!remoteDllPath) {
    return std::unexpected{remoteDllPath.error()};
  }

  std::expected<LPTHREAD_START_ROUTINE, std::error_code> remoteLoadLibraryAddr{
      GetRemoteLoadLibraryAddr()};
  if (!remoteLoadLibraryAddr) {
    return std::unexpected{remoteLoadLibraryAddr.error()};
  }

  const ScopedHandle kRemoteThread{CreateRemoteThread(
      /*hProcess           = */ proc,
      /*lpThreadAttributes = */ nullptr,
      /*dwStackSize        = */ 0,
      /*lpStartAddress     = */ *remoteLoadLibraryAddr,
      /*lpParameter        = */ remoteDllPath->Get(),
      /*dwCreationFlags    = */ 0,
      /*lpThreadId         = */ nullptr)};
  if (!kRemoteThread) {
    return std::unexpected{LastError()};
  }

  const DWORD kWaitResult{WaitForSingleObject(
      /*hHandle        = */ kRemoteThread.Get(),
      /*dwMilliseconds = */ INFINITE)};
  if (kWaitResult == WAIT_FAILED) {
    return std::unexpected{LastError()};
  }

  return {};
}

}  // namespace afw::remote
