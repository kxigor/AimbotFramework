#include <windows.h>

#include <afw/win/scoped_handle.hpp>

static DWORD WINAPI OnAttach(LPVOID module) {
  MessageBoxW(
      /*hWnd      = */ nullptr,
      /*lpText    = */
      L"aimbot.dll is injected and running inside this process.",
      /*lpCaption = */ L"AimbotFramework",
      /*uType     = */ MB_OK | MB_ICONINFORMATION);
  FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
  return 0;
}

BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
  (void)reserved;
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(instance);
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
