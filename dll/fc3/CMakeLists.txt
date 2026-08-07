# =========================================================
# FAR CRY 3 SHIM  (aimbot_fc3.dll)
# =========================================================
# The injectable Windows library for one title: DllMain hosting afw::app with
# a games::fc3 binding behind it. Produces "aimbot_fc3.dll" (PREFIX "" strips
# the MinGW "lib" prefix). The name carries the title because a second one will
# sit beside it, and because the injector picks the library by name.
#
# Far Cry 3 is a 32-bit process, so this target is only useful from the x86
# presets; see CMakePresets.json.

if(NOT WIN32)
  message(FATAL_ERROR "The DLL shim targets Windows. Configure under MSYS2 UCRT64 (or MSVC).")
endif()

add_library(aimbot_fc3 SHARED
  dllmain.cpp
  farcry3.cpp
  target.cpp
  assist.cpp
  steady.cpp
  probe.cpp
)

set_target_properties(aimbot_fc3 PROPERTIES
  PREFIX ""            # -> aimbot_fc3.dll, not libaimbot_fc3.dll
  OUTPUT_NAME "aimbot_fc3"
)

target_link_libraries(aimbot_fc3
  PRIVATE
    afw::aimbot
    afw::app          # the session: overlay, input, unload; pulls in afw::hook
    afw::games::fc3   # pulls in afw::process transitively
    afw::esp          # world-to-screen boxes drawn from the frame callback
    afw::win          # RAII wrappers (ScopedHandle) used in DllMain
)

# Dev-time interface targets are optional (see root CMakeLists).
if(TARGET project_options)
  target_link_libraries(aimbot_fc3 PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(aimbot_fc3 PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(aimbot_fc3 PRIVATE project_coverage)
endif()

# Statically link the MinGW runtime (libgcc / libstdc++ / winpthread) INTO the
# DLL. Otherwise the injected module would need those MSYS2 runtime DLLs on the
# host process's search path, which it will not have. MSVC links its runtime
# differently, so this is GNU/Clang-on-Windows only.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(aimbot_fc3 PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
    -Wl,--exclude-all-symbols   # export nothing implicitly; keep the surface minimal
  )
endif()
