#include <windows.h>

#include <afw/app/app.hpp>
#include <afw/win/scoped_handle.hpp>

#include "farcry3.hpp"

namespace {

DWORD WINAPI OnAttach(LPVOID /*parameter*/) {
  afw::dll::FarCry3 game{};
  afw::app::Run(game);
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
        /*lpParameter        = */ nullptr,
        /*dwCreationFlags    = */ 0,
        /*lpThreadId         = */ nullptr)};
  }
  return TRUE;
}
