add_gtest(hook_unit
  SOURCES
    vmt_hook_unit.cpp
    error_unit.cpp
  LIBRARIES afw::hook afw::win project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)
