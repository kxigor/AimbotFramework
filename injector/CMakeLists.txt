# =========================================================
# INJECTOR  (injector.exe)
# =========================================================
# Standalone tool that finds the target process and loads aimbot.dll into it
# through afw::remote. Windows-only; the MinGW runtime is linked statically so
# the executable is self-contained (same rationale as the DLL shim).

if(NOT WIN32)
  message(FATAL_ERROR "The injector targets Windows. Configure under MSYS2 UCRT64 (or MSVC).")
endif()

add_executable(afw_injector
  injector.cpp
)
set_target_properties(afw_injector PROPERTIES
  OUTPUT_NAME "injector"
)

target_link_libraries(afw_injector PRIVATE afw::remote afw::win)

# Dev-time interface targets are optional (see root CMakeLists).
if(TARGET project_options)
  target_link_libraries(afw_injector PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_injector PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_injector PRIVATE project_coverage)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(afw_injector PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
  )
endif()

# Stage the freshly built aimbot.dll next to injector.exe so ResolveDllPath (it
# looks in the executable's own directory) can find and inject it at runtime.
#
# A target of its own rather than POST_BUILD on the injector: a POST_BUILD command
# only runs when its own target is relinked, so a change confined to the DLL left
# a stale copy staged and the injector kept loading the previous build.
if(TARGET aimbot)
  add_custom_target(afw_stage_dll ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:aimbot> $<TARGET_FILE_DIR:afw_injector>
    COMMENT "Staging aimbot.dll next to injector.exe"
    VERBATIM
  )
  add_dependencies(afw_stage_dll aimbot afw_injector)
endif()
