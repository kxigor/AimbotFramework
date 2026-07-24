add_gtest(BasicUnit
  SOURCES BasicUnit.cpp
  LIBRARIES afw::process afw::games::fc3 afw::aimbot project_options project_sanitizers project_coverage
)
