# cmake/TidyTools.cmake

# ========================================
# 1. CLANG-TIDY AND PYTHON SEARCH
# ========================================
# Finds clang-tidy and the Python 3 interpreter (the analysis is driven by a
# Python wrapper script). Both are required to register the 'TidyCheck' target.
#
# MSYS2 note: the unsuffixed 'clang-tidy' is searched FIRST (that is what the
# LLVM package installs); versioned names stay as a Linux fallback.

include_guard()

message(STATUS "Searching for static analysis tools: clang-tidy and Python3 ...")

find_program(CLANG_TIDY_EXE
  NAMES
    clang-tidy
    clang-tidy-24
    clang-tidy-23
    clang-tidy-22
    clang-tidy-21
    clang-tidy-20
    clang-tidy-19
  DOC "Path to clang-tidy for static analysis."
)

find_package(Python3 QUIET COMPONENTS Interpreter)

# ========================================
# 2. STATUS AND FINAL LOGIC
# ========================================
if(CLANG_TIDY_EXE)
  message(STATUS "Clang-Tidy found: ${CLANG_TIDY_EXE}")
else()
  message(WARNING "Clang-Tidy not found. Static analysis checks will be disabled.")
endif()

if(Python3_FOUND)
  message(STATUS "Python 3 found.")
else()
  message(WARNING "Python 3 (required for the analysis script) not found.")
endif()

if(CLANG_TIDY_EXE AND Python3_FOUND)
  message(STATUS "Result: TidyCheck targets are FULLY enabled.")
elseif(CLANG_TIDY_EXE AND NOT Python3_FOUND)
  message(WARNING "Result: TidyCheck targets disabled due to missing Python 3 dependency.")
  unset(CLANG_TIDY_EXE)
else()
  message(STATUS "Result: TidyCheck targets are skipped.")
  unset(CLANG_TIDY_EXE)
endif()
