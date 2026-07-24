# cmake/ProjectSanitizers.cmake

# ========================================
# 1. ARCHITECTURE SETUP & PURPOSE
# ========================================
# Creates the 'project_sanitizers' INTERFACE library carrying the flags for the
# sanitizer selected via SANITIZER_MODE.
#
# Platform reality on Windows / MSYS2 (this project's target):
#   * MinGW GCC ships NO sanitizer runtimes at all (no libasan/libtsan/…).
#   * Clang on Windows supports Address + Undefined only.
#   * MSVC supports Address only.
#   * Thread and Memory sanitizers do not exist on Windows for any toolchain.
# So instead of emitting flags the linker cannot satisfy, we validate the
# requested mode against the active compiler/platform and fall back to "None"
# with a clear message when it is unsupported.

include_guard()

add_library(project_sanitizers INTERFACE)

# ========================================
# 2. MODE CONFIGURATION & USER OPTIONS
# ========================================
set(SANITIZER_MODE "None" CACHE STRING "Sanitizer mode: None, Address, Thread, Memory, Undefined")
set_property(CACHE SANITIZER_MODE PROPERTY STRINGS "None" "Address" "Thread" "Memory" "Undefined")

# ========================================
# 3. AVAILABILITY GATING
# ========================================
# Determine whether the requested SANITIZER_MODE is actually supported here.
set(_san_supported TRUE)
set(_san_reason "")

if(SANITIZER_MODE STREQUAL "None")
  # Nothing to do.
elseif(WIN32)
  # Windows: only Clang/MSVC + Address, and Clang + Undefined, are real.
  if(SANITIZER_MODE STREQUAL "Thread" OR SANITIZER_MODE STREQUAL "Memory")
    set(_san_supported FALSE)
    set(_san_reason "${SANITIZER_MODE}Sanitizer does not exist on Windows")
  elseif(SANITIZER_MODE STREQUAL "Address")
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
      set(_san_supported FALSE)
      set(_san_reason "AddressSanitizer on Windows needs Clang or MSVC (MinGW GCC has no ASan runtime)")
    endif()
  elseif(SANITIZER_MODE STREQUAL "Undefined")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(_san_supported FALSE)
      set(_san_reason "UBSan is reliable only under Clang on Windows (MinGW GCC lacks the runtime)")
    endif()
  endif()
endif()

if(NOT _san_supported)
  message(WARNING
    "SANITIZER_MODE='${SANITIZER_MODE}' is unavailable: ${_san_reason}. "
    "Falling back to None.")
  set(SANITIZER_MODE "None" CACHE STRING "Sanitizer mode (forced to None: unsupported here)" FORCE)
endif()

# ========================================
# 4. SANITIZER PRESETS (FLAGS)
# ========================================
set(UBSAN_FLAGS
  -fsanitize=undefined
  -fno-sanitize-recover=undefined
  -fno-omit-frame-pointer)

set(ASAN_FLAGS
  -fsanitize=address
  ${UBSAN_FLAGS})

set(TSAN_FLAGS
  -fsanitize=thread
  ${UBSAN_FLAGS})

set(MSAN_FLAGS
  -fsanitize=memory
  -fsanitize-memory-track-origins
  ${UBSAN_FLAGS})

set(MSVC_ASAN_FLAGS /fsanitize=address)

# ========================================
# 5. APPLYING LOGIC VIA GENERATOR EXPRESSIONS
# ========================================
if(NOT SANITIZER_MODE STREQUAL "None")
  list(APPEND SAN_APPLIED_FLAGS
    $<$<AND:$<STREQUAL:${SANITIZER_MODE},Address>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>:${ASAN_FLAGS}>
    $<$<AND:$<STREQUAL:${SANITIZER_MODE},Address>,$<CXX_COMPILER_ID:MSVC>>:${MSVC_ASAN_FLAGS}>

    $<$<AND:$<STREQUAL:${SANITIZER_MODE},Thread>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>:${TSAN_FLAGS}>
    $<$<AND:$<STREQUAL:${SANITIZER_MODE},Memory>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>:${MSAN_FLAGS}>
    $<$<AND:$<STREQUAL:${SANITIZER_MODE},Undefined>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>:${UBSAN_FLAGS}>
  )

  target_compile_options(project_sanitizers INTERFACE ${SAN_APPLIED_FLAGS})
  target_link_options(project_sanitizers INTERFACE ${SAN_APPLIED_FLAGS})
endif()
