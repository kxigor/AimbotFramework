add_gtest(basic_unit
  SOURCES basic_unit.cpp
  LIBRARIES afw::process afw::games::fc3 afw::aimbot project_options project_sanitizers project_coverage
)
