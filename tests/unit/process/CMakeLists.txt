add_gtest(process_unit
  SOURCES
    memory_unit.cpp
    reader_unit.cpp
    error_unit.cpp
  LIBRARIES afw::process afw::math project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)
