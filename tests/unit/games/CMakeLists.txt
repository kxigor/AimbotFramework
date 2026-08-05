add_gtest(games_unit
  SOURCES
    census_unit.cpp
    snapshot_unit.cpp
    handoff_unit.cpp
    bones_unit.cpp
    classify_unit.cpp
    error_unit.cpp
  LIBRARIES afw::games::fc3 project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)
