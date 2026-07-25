# =========================================================
# DLL SHIM  (aimbot.dll)
# =========================================================
# The injectable Windows library: DllMain hosting afw::aimbot.
# Produces "aimbot.dll" (PREFIX "" strips the MinGW "lib" prefix).

if(NOT WIN32)
  message(FATAL_ERROR "The DLL shim targets Windows. Configure under MSYS2 UCRT64 (or MSVC).")
endif()

add_library(aimbot SHARED
  dllmain.cpp
)

set_target_properties(aimbot PROPERTIES
  PREFIX ""            # -> aimbot.dll, not libaimbot.dll
  OUTPUT_NAME "aimbot"
)

target_link_libraries(aimbot
  PRIVATE
    afw::aimbot
    afw::games::fc3   # pulls in afw::process transitively
    afw::win          # RAII wrappers (ScopedHandle) used in DllMain
)

# Dev-time interface targets are optional (see root CMakeLists).
if(TARGET project_options)
  target_link_libraries(aimbot PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(aimbot PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(aimbot PRIVATE project_coverage)
endif()

# Statically link the MinGW runtime (libgcc / libstdc++ / winpthread) INTO the
# DLL. Otherwise the injected module would need those MSYS2 runtime DLLs on the
# host process's search path, which it will not have. MSVC links its runtime
# differently, so this is GNU/Clang-on-Windows only.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(aimbot PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
    -Wl,--exclude-all-symbols   # export nothing implicitly; keep the surface minimal
  )
endif()
