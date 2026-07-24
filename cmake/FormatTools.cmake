# cmake/FormatTools.cmake

# ========================================
# 1. CLANG-FORMAT TOOL SEARCH
# ========================================
# Finds the clang-format executable used by the 'FormatCheck' CTest target.
#
# MSYS2 note: the LLVM packages install an unsuffixed 'clang-format' (no
# 'clang-format-19'..'-24'), so the plain name is searched FIRST; the versioned
# names remain as a fallback for Linux distros that only ship suffixed binaries.

include_guard()

message(STATUS "Searching for code formatting tool: clang-format...")

find_program(CLANG_FORMAT_EXE
  NAMES
    clang-format
    clang-format-24
    clang-format-23
    clang-format-22
    clang-format-21
    clang-format-20
    clang-format-19
  DOC "Path to clang-format for formatting."
)

# ========================================
# 2. TOOL STATUS LOGGING
# ========================================
if(CLANG_FORMAT_EXE)
  message(STATUS "Clang-Format found: ${CLANG_FORMAT_EXE}. FormatCheck target is enabled.")
else()
  message(WARNING "Clang-Format not found. FormatCheck target will be disabled.")
endif()
