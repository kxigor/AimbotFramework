#include <windows.h>

#include <afw/win/scoped_handle.hpp>

namespace {

DWORD WINAPI OnAttach(LPVOID module) {
  MessageBoxW(
      /*hWnd      = */ nullptr,
      /*lpText    = */
      L"aimbot.dll is injected and running inside this process.",
      /*lpCaption = */ L"AimbotFramework",
      /*uType     = */ MB_OK | MB_ICONINFORMATION);
  FreeLibraryAndExitThread(
      /*hLibModule = */ static_cast<HMODULE>(module),
      /*dwExitCode = */ 0);
  return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID /*reserved*/) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(
        /*hLibModule = */ instance);
    const afw::win::ScopedHandle kThread{CreateThread(
        /*lpThreadAttributes = */ nullptr,
        /*dwStackSize        = */ 0,
        /*lpStartAddress     = */ OnAttach,
        /*lpParameter        = */ instance,
        /*dwCreationFlags    = */ 0,
        /*lpThreadId         = */ nullptr)};
  }
  return TRUE;
}
