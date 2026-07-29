add_gtest(win_unit
  SOURCES
    scoped_handle_unit.cpp
    remote_memory_unit.cpp
    protect_guard_unit.cpp
    error_unit.cpp
  LIBRARIES afw::win project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)
