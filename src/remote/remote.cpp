#include <tlhelp32.h>
#include <windows.h>

#include <afw/remote/remote.hpp>
#include <cstddef>

namespace afw::remote {

DWORD GetPidByProcName(PCWSTR procName) {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return 0;
  }

  PROCESSENTRY32W entry = {.dwSize = sizeof(PROCESSENTRY32W)};
  DWORD pid = -1;

  if (Process32FirstW(snapshot, &entry)) {
    do {
      if (_wcsicmp(entry.szExeFile, procName) == 0) {
        pid = entry.th32ProcessID;
        break;
      }
    } while (Process32NextW(snapshot, &entry));
  }

  CloseHandle(snapshot);
  return pid;
}

LPVOID StrDupEx(HANDLE proc, PCWSTR dllPath) {
  const std::size_t kDllPathByteSize = (wcslen(dllPath) + 1) * sizeof(WCHAR);

  LPVOID remoteDllPath = VirtualAllocEx(
    proc, 
    nullptr, 
    kDllPathByteSize,
    MEM_COMMIT, 
    PAGE_READWRITE
  );

  if (remoteDllPath == nullptr) {
    return nullptr;
  }

  bool writeStatus = WriteProcessMemory(
    proc, 
    remoteDllPath, 
    dllPath,
    kDllPathByteSize, 
    nullptr
  );

  if (!writeStatus) {
    return nullptr;
  }

  return remoteDllPath;
}

HMODULE GetRemoteLoadLibraryAddr() {
  HMODULE k32 = GetModuleHandleA("kernel32.dll");
  HMODULE remoteLoadLibraryAddr = GetProcAddress(k32, "LoadLibraryW");
  return remoteLoadLibraryAddr;
}

bool LoadDll(HANDLE proc, PCWSTR dllPath) {
  LPVOID remoteDllPath = StrDupEx(proc, dllPath);

  LPTHREAD_START_ROUTINE remoteLoadLibraryAddr =
      static_cast<LPTHREAD_START_ROUTINE>(GetRemoteLoadLibraryAddr());

  HANDLE remoteThread = CreateRemoteThread(
      /*hProcess            = */ process,
      /*lpThreadAttributes  = */ NULL,
      /*dwStackSize         = */ NULL,
      /*lpStartAddress      = */ remoteLoadLibraryAddr,
      /*lpParameter         = */ remoteString,
      /*dwCreationFlags     = */ NULL,
      /*lpThreadId          = */ NULL);

  WaitForSingleObject(
    /*hHandle         = */ thread, 
    /*dwMilliseconds  = */ INFINITE
  );

  CloseHandle(
    /*hObject = */ thread
  );

}

};  // namespace afw::remote