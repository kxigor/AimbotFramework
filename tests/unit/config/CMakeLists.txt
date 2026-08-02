add_gtest(config_unit
  SOURCES
    config_unit.cpp
  LIBRARIES afw::config project_options project_sanitizers project_coverage
  PROPERTIES TIMEOUT 30
)
