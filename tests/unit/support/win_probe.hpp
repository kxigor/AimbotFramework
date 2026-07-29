#pragma once

#include <windows.h>

#include <string>

namespace afw::test {

[[nodiscard]] inline bool HandleIsClosed(HANDLE handle) {
  DWORD flags{};
  SetLastError(/*dwErrCode = */ ERROR_SUCCESS);
  if (GetHandleInformation(
          /*hObject   = */ handle,
          /*lpdwFlags = */ &flags) != FALSE) {
    return false;
  }
  return GetLastError() == ERROR_INVALID_HANDLE;
}

[[nodiscard]] inline std::wstring ProbeName(PCWSTR tag) {
  return L"Local\\afw_test_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
         std::wstring{tag};
}

[[nodiscard]] inline HANDLE CreateProbeEvent(const std::wstring& name) {
  return CreateEventW(
      /*lpEventAttributes = */ nullptr,
      /*bManualReset      = */ TRUE,
      /*bInitialState     = */ FALSE,
      /*lpName            = */ name.c_str());
}

[[nodiscard]] inline bool ProbeObjectExists(const std::wstring& name) {
  const HANDLE kProbe{OpenEventW(
      /*dwDesiredAccess = */ SYNCHRONIZE,
      /*bInheritHandle  = */ FALSE,
      /*lpName          = */ name.c_str())};
  if (kProbe == nullptr) {
    return false;
  }
  CloseHandle(
      /*hObject = */ kProbe);
  return true;
}

[[nodiscard]] inline bool RegionIsFree(HANDLE proc, LPCVOID addr) {
  MEMORY_BASIC_INFORMATION info{};
  const SIZE_T kQueried{VirtualQueryEx(
      /*hProcess  = */ proc,
      /*lpAddress = */ addr,
      /*lpBuffer  = */ &info,
      /*dwLength  = */ sizeof(info))};
  return kQueried == sizeof(info) && info.State == MEM_FREE;
}

[[nodiscard]] inline bool RegionHasProtection(HANDLE proc, LPCVOID addr,
                                              DWORD protect) {
  MEMORY_BASIC_INFORMATION info{};
  const SIZE_T kQueried{VirtualQueryEx(
      /*hProcess  = */ proc,
      /*lpAddress = */ addr,
      /*lpBuffer  = */ &info,
      /*dwLength  = */ sizeof(info))};
  return kQueried == sizeof(info) && info.Protect == protect;
}

[[nodiscard]] inline bool RegionIsCommitted(HANDLE proc, LPCVOID addr) {
  MEMORY_BASIC_INFORMATION info{};
  const SIZE_T kQueried{VirtualQueryEx(
      /*hProcess  = */ proc,
      /*lpAddress = */ addr,
      /*lpBuffer  = */ &info,
      /*dwLength  = */ sizeof(info))};
  return kQueried == sizeof(info) && info.State == MEM_COMMIT;
}

}  // namespace afw::test
