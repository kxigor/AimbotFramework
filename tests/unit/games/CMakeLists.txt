add_gtest(games_unit
  SOURCES
    fc3_unit.cpp
    error_unit.cpp
  LIBRARIES afw::games::fc3 project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)
