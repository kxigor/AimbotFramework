# =========================================================
# INJECTOR  (injector.exe)
# =========================================================
# Standalone tool that finds the target process and loads the shim into it
# through afw::remote. Windows-only; the MinGW runtime is linked statically so
# the executable is self-contained (same rationale as the DLL shim).
#
# It knows one title: kTargetImageName and kDllName in injector.cpp name Far
# Cry 3 and aimbot_fc3.dll. A second title needs the pair chosen at runtime,
# not a second executable.

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

# Stage the freshly built shim next to injector.exe so ResolveDllPath (it looks
# in the executable's own directory) can find and inject it at runtime.
#
# A target of its own rather than POST_BUILD on the injector: a POST_BUILD command
# only runs when its own target is relinked, so a change confined to the DLL left
# a stale copy staged and the injector kept loading the previous build.
#
# The staged copy is stripped of its debug information while the build tree
# keeps a full one. This is not about disk: a PE is mapped at its SizeOfImage,
# and DWARF takes that from 2.3 MB to 23.6 MB, which the loader has to find as
# one free run inside a 32-bit game already holding a 37 MB engine module and
# ~190 MB of display driver. Past roughly 20 MB the remote LoadLibraryW starts
# returning NULL.
if(TARGET aimbot_fc3)
  set(afw_staged_dll
      $<TARGET_FILE_DIR:afw_injector>/$<TARGET_FILE_NAME:aimbot_fc3>)

  add_custom_target(afw_stage_dll ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:aimbot_fc3> $<TARGET_FILE_DIR:afw_injector>
    COMMENT "Staging aimbot_fc3.dll next to injector.exe"
    VERBATIM
  )

  if(CMAKE_STRIP)
    add_custom_command(TARGET afw_stage_dll POST_BUILD
      COMMAND ${CMAKE_STRIP} --strip-debug ${afw_staged_dll}
      COMMENT "Stripping debug information from the staged aimbot_fc3.dll"
      VERBATIM
    )
  endif()

  add_dependencies(afw_stage_dll aimbot_fc3 afw_injector)
endif()
