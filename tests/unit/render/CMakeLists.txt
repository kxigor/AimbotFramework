add_gtest(RenderUnit
  SOURCES ProbeSwapChainVtableUnit.cpp
  LIBRARIES afw::render project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 60
)
