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
#   afw::win          RAII wrappers over raw Win32 resources (handles, memory)
#   afw::remote       Windows DLL-injection helpers (Toolhelp + CreateRemoteThread)
#
# Dependency direction: games::* -> process ; the DLL shim wires a game into the
# aimbot. None contains a Windows entry point. process / games / aimbot are still
# empty skeletons; afw::remote is implemented and Windows-only.

add_subdirectory(process)
add_subdirectory(games)
add_subdirectory(aimbot)

# Windows-only libraries; skip on non-Windows so afw::core still configures for
# a Linux/macOS consumer. win is added before remote, which links against it.
if(WIN32)
  add_subdirectory(win)
  add_subdirectory(remote)
endif()
