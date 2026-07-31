add_gtest(render_unit
  SOURCES
    render_unit.cpp
    present_hook_unit.cpp
  LIBRARIES afw::render project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 60
)
