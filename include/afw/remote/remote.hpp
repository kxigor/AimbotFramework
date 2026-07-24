#pragma once

#include <windows.h>

namespace afw::remote {
DWORD GetPidByProcName(PCWSTR procName);
bool LoadDll(HANDLE process, PCWSTR dllPath);
};  // namespace afw::remote