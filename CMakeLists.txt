cmake_minimum_required(VERSION 3.25)

project(aimbot-framework LANGUAGES CXX)

# =========================================================
# 0. STANDALONE vs SUBPROJECT
# =========================================================
# When consumed via add_subdirectory / FetchContent, dev-time machinery (DLL,
# tests, QA checks, sanitizers, coverage) stays disabled by default and the
# parent project keeps control of global settings.

if(PROJECT_IS_TOP_LEVEL)
  set(AFW_DEV_DEFAULT ON)
else()
  set(AFW_DEV_DEFAULT OFF)
endif()

# The DLL shim is Windows-only; only default it ON when we are top-level AND on
# Windows so a Linux/macOS consumer of the libraries does not trip its guard.
set(AFW_DLL_DEFAULT OFF)
if(AFW_DEV_DEFAULT AND WIN32)
  set(AFW_DLL_DEFAULT ON)
endif()

# =========================================================
# 1. OPTIONS (prefixed AFW_ to keep cache names unique)
# =========================================================
option(AFW_BUILD_DLL          "Build the injectable aimbot.dll shim" ${AFW_DLL_DEFAULT})
option(AFW_BUILD_INJECTOR     "Build the standalone injector.exe"    ${AFW_DLL_DEFAULT})
option(AFW_BUILD_TESTS        "Build the GTest test suite"           ${AFW_DEV_DEFAULT})
option(AFW_ENABLE_FORMAT_CHECK "Register clang-format as CTest"      ${AFW_DEV_DEFAULT})
option(AFW_ENABLE_TIDY_CHECK   "Register clang-tidy as CTest"        ${AFW_DEV_DEFAULT})
option(AFW_ENABLE_COVERAGE     "Enable gcov coverage targets"        OFF)

# Format/tidy/coverage only make sense alongside tests.
if(NOT AFW_BUILD_TESTS)
  set(AFW_ENABLE_FORMAT_CHECK OFF CACHE BOOL "Disabled: requires AFW_BUILD_TESTS=ON" FORCE)
  set(AFW_ENABLE_TIDY_CHECK   OFF CACHE BOOL "Disabled: requires AFW_BUILD_TESTS=ON" FORCE)
  set(AFW_ENABLE_COVERAGE     OFF CACHE BOOL "Disabled: requires AFW_BUILD_TESTS=ON" FORCE)
endif()

# =========================================================
# 2. TOP-LEVEL-ONLY GLOBAL SETTINGS
# =========================================================
if(PROJECT_IS_TOP_LEVEL)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
  endif()
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# A preset that targets one architecture states its pointer size here, so that
# configuring it from the wrong MSYS2 shell is an error rather than a build of
# the other architecture into a directory named for this one.
if(AFW_TARGET_POINTER_SIZE AND
   NOT CMAKE_SIZEOF_VOID_P EQUAL AFW_TARGET_POINTER_SIZE)
  math(EXPR AFW_WANTED_BITS "${AFW_TARGET_POINTER_SIZE} * 8")
  math(EXPR AFW_FOUND_BITS "${CMAKE_SIZEOF_VOID_P} * 8")
  message(FATAL_ERROR
    "This preset targets ${AFW_WANTED_BITS}-bit but ${CMAKE_CXX_COMPILER} "
    "produces ${AFW_FOUND_BITS}-bit code.\n"
    "Open the matching MSYS2 shell (MINGW32 for 32-bit, UCRT64 for 64-bit) "
    "and configure again."
  )
endif()

# =========================================================
# 3. DEV-ONLY MACHINERY (skipped for subproject consumers)
# =========================================================
# Included before the subdirectories so the project_options / project_sanitizers
# / project_coverage interface targets exist when src/ and dll/ link them.
if(AFW_BUILD_DLL OR AFW_BUILD_TESTS OR AFW_ENABLE_COVERAGE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  include(ProjectOptions)
  include(ProjectSanitizers)
  include(ProjectCoverage)
endif()

# =========================================================
# 4. SUBDIRECTORIES
# =========================================================
if(WIN32)
  add_subdirectory(third_party)  # Dear ImGui submodule + Win32/D3D11 backends
endif()

add_subdirectory(src)          # the afw::* libraries (always)

if(AFW_BUILD_DLL)
  add_subdirectory(dll)        # aimbot.dll shim
endif()

if(AFW_BUILD_INJECTOR)
  add_subdirectory(injector)   # injector.exe -> loads aimbot.dll via afw::remote
endif()

if(AFW_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
