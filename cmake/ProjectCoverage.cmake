# cmake/ProjectCoverage.cmake

# ========================================
# 1. CODE COVERAGE ARCHITECTURE SETUP
# ========================================
# Creates the 'project_coverage' INTERFACE library that applies gcov
# instrumentation only when AFW_ENABLE_COVERAGE is ON (a no-op otherwise).
#
# Portability note: gcov itself ships with MinGW GCC and works on Windows /
# MSYS2. The *reporting* tools do not always: lcov/genhtml and their Unix path
# filters ('/usr/*') are frequently absent under MSYS2. So report generation is
# optional and tool-driven: gcovr first (single cross-platform binary), then
# lcov+genhtml, otherwise a graceful skip. Coverage instrumentation still works
# even with no reporter installed (raw .gcda/.gcno are produced).

include_guard()

add_library(project_coverage INTERFACE)

if(AFW_ENABLE_COVERAGE)
  # gcov coverage is a GCC/Clang feature only.
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(WARNING "AFW_ENABLE_COVERAGE=ON but compiler '${CMAKE_CXX_COMPILER_ID}' has no gcov support. Coverage disabled.")
    return()
  endif()

  set(COVERAGE_FLAGS "--coverage")

  target_compile_options(project_coverage INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:${COVERAGE_FLAGS}>
  )
  target_link_options(project_coverage INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:${COVERAGE_FLAGS}>
  )

  # ========================================
  # 2. REPORT TARGET (tool-driven, optional)
  # ========================================
  set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage_report")

  find_program(GCOVR_EXECUTABLE NAMES gcovr)
  find_program(LCOV_EXECUTABLE NAMES lcov)
  find_program(GENHTML_EXECUTABLE NAMES genhtml)

  if(GCOVR_EXECUTABLE)
    message(STATUS "Coverage: found gcovr (${GCOVR_EXECUTABLE}). 'coverage' target -> HTML via gcovr.")
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
      COMMAND ${GCOVR_EXECUTABLE}
              --root ${PROJECT_SOURCE_DIR}
              --filter ${PROJECT_SOURCE_DIR}/src
              --filter ${PROJECT_SOURCE_DIR}/dll
              --html-details ${COVERAGE_OUTPUT_DIR}/index.html
              --print-summary
              ${CMAKE_BINARY_DIR}
      COMMENT "Running tests and generating gcovr HTML report in ${COVERAGE_OUTPUT_DIR}"
      USES_TERMINAL
    )
  elseif(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    message(STATUS "Coverage: found lcov/genhtml. 'coverage' target -> HTML via lcov.")
    set(LCOV_INFO_FILE "${COVERAGE_OUTPUT_DIR}/coverage.info")
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
      COMMAND ${LCOV_EXECUTABLE} --zerocounters --directory ${CMAKE_BINARY_DIR}
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
      COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR} --output-file ${LCOV_INFO_FILE}
      # Portable filter: keep only files under the project source tree instead of
      # relying on Unix '/usr/*' paths that do not exist on MSYS2.
      COMMAND ${LCOV_EXECUTABLE} --extract ${LCOV_INFO_FILE} "${PROJECT_SOURCE_DIR}/*" --output-file ${LCOV_INFO_FILE}
      COMMAND ${GENHTML_EXECUTABLE} ${LCOV_INFO_FILE} --output-directory ${COVERAGE_OUTPUT_DIR}
      COMMENT "Running tests and generating lcov HTML report in ${COVERAGE_OUTPUT_DIR}"
      USES_TERMINAL
    )
  else()
    message(STATUS "Coverage: instrumentation ON, but no reporter (gcovr / lcov+genhtml) found. "
                   "Raw .gcda/.gcno will be produced; 'coverage' target skipped.")
  endif()
endif()
