#include <windows.h>

#include <afw/remote/remote.hpp>

static constexpr PCWSTR kProcName = L"notepad.exe";
static constexpr PCWSTR kDllPath = L"123";

int main() {
  DWORD pid = afw::remote::GetPidByProcName(kProcName);
  HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
  return afw::remote::LoadDll(proc, kDllPath);
}