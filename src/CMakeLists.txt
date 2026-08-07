# =========================================================
# LIBRARIES
# =========================================================
# Reusable logic split into focused static libraries, layered by dependency.
# Public headers live one level up, in the top-level include/ (sibling of src/),
# so every library shares the same include root: include/afw/...
#
#   afw::process      generic access to the host process (memory, modules, scan)
#   afw::games::<t>   per-title, engine-specific bindings; depend on afw::process
#   afw::aimbot       target choice and the geometry of pointing at one
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
# None contains a Windows entry point.
#
# WHAT IS NOT YET TITLE-NEUTRAL, stated plainly so a second title does not
# discover it the hard way:
#
#   afw::esp     links afw::games::fc3 and names EntityInfo, Kind, CameraBasis
#                and Diagnostics in its own internal/ headers. It reads more of
#                an entity than the aimbot does -- the transform and the box for
#                the outlines, the head and spine for the aim box -- so it is
#                the harder of the two to lift off a title, not the easier.
#   afw::render  is Direct3D 11 and DXGI. Only include/afw/render/overlay.hpp is
#                API-neutral; everything naming IDXGISwapChain lives under
#                render/dxgi/. A title on OpenGL or Vulkan needs a sibling
#                backend, not a change here.
#   afw::app     knows no game, but it does drive the Present hook directly
#                through afw::render::dxgi, so it inherits that backend.
#   afw::config  carries Far Cry 3's taxonomy (allies, civilians, animals,
#                birds, vehicles) in a Fc3Settings section. Enemies and props
#                stay at the top level because every title has both.

add_subdirectory(config)
add_subdirectory(math)

# Windows-only libraries; skip on non-Windows so the rest still configures for
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
