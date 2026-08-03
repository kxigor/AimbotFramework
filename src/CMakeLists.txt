# =========================================================
# LIBRARIES
# =========================================================
# Reusable logic split into focused static libraries, layered by dependency.
# Public headers live one level up, in the top-level include/ (sibling of src/),
# so every library shares the same include root: include/afw/...
#
#   afw::process      generic access to the host process (memory, modules, scan)
#   afw::games::<t>   per-title, engine-specific bindings; depend on afw::process
#   afw::aimbot       the aim-assist logic (engine-agnostic)
#   afw::config       settings shared by the overlay's consumers
#   afw::win          RAII wrappers over raw Win32 resources (handles, memory)
#   afw::remote       Windows DLL-injection helpers (Toolhelp + CreateRemoteThread)
#   afw::hook         function interception (virtual method table, window proc)
#   afw::render       owns the Present hook; the overlay's foundation
#   afw::menu         the overlay's user interface
#   afw::esp          entity boxes projected into screen space
#   afw::app          the injected DLL's session, with no title-specific parts
#
# Dependency direction: games::* -> process ; render -> hook -> win ; menu and
# esp are siblings on top of render, sharing state only through config, so
# neither one requires the other to run. afw::app sits on top of render and
# menu and knows no game; the DLL shim supplies one through afw::app::Game.
# None contains a Windows entry point. process / games / aimbot are still empty
# skeletons; afw::remote is implemented and Windows-only.

add_subdirectory(config)
add_subdirectory(math)

# Windows-only libraries; skip on non-Windows so afw::core still configures for
# a Linux/macOS consumer. Ordered by dependency: win before remote and hook,
# hook before render, render before its two consumers. process reads this
# process's own memory through the Win32 API, so games and aimbot, which build
# on it, are Windows-only too.
if(WIN32)
  add_subdirectory(win)
  add_subdirectory(remote)
  add_subdirectory(hook)
  add_subdirectory(process)
  add_subdirectory(games)
  add_subdirectory(aimbot)
  add_subdirectory(render)
  add_subdirectory(menu)
  add_subdirectory(esp)
  add_subdirectory(app)
endif()
