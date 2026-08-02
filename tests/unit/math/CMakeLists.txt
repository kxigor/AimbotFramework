add_gtest(math_unit
  SOURCES
    vec3_unit.cpp
    aabb_unit.cpp
    matrix4_unit.cpp
  LIBRARIES afw::math project_options project_sanitizers project_coverage
  PROPERTIES TIMEOUT 30
)
