# =========================================================
# afw::remote  (STATIC)
# =========================================================
# Windows DLL-injection helpers: find a process by name and load a library into
# it via the classic CreateRemoteThread + LoadLibraryW technique (Toolhelp +
# remote-thread APIs). Consumed by the injector executable. Windows-only, so it
# is added from src/ only under WIN32.

add_library(afw_remote STATIC
  remote.cpp
  error.cpp
)
add_library(afw::remote ALIAS afw_remote)

target_compile_features(afw_remote PUBLIC cxx_std_23)

# Public headers live in the top-level include/ (sibling of src/).
target_include_directories(afw_remote
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# remote.hpp returns an afw::win::RemoteMemory from StrDupEx, so afw::win is
# part of this library's public interface and has to propagate to consumers.
target_link_libraries(afw_remote PUBLIC afw::win)

if(TARGET project_options)
  target_link_libraries(afw_remote PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_remote PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_remote PRIVATE project_coverage)
endif()
