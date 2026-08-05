add_gtest(esp_unit
  SOURCES
    classify_unit.cpp
  LIBRARIES afw::esp project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)
