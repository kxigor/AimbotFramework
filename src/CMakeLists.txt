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
#
# Dependency direction: games::* -> process ; the DLL shim wires a game into the
# aimbot. None contains a Windows entry point. All are empty skeletons.

add_subdirectory(process)
add_subdirectory(games)
add_subdirectory(aimbot)
