add_gtest(remote_unit
  SOURCES
    remote_lookup_unit.cpp
    error_unit.cpp
  LIBRARIES afw::remote afw::win project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)

# Kept in its own binary: it performs a real CreateRemoteThread injection, which
# some endpoint-protection products block and which can in principle hang. A
# failure here does not take the rest of afw::remote's results with it, and the
# label allows `ctest -LE injection`.
add_gtest(remote_inject_unit
  SOURCES remote_inject_unit.cpp
  LIBRARIES afw::remote afw::win project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 60 LABELS injection
  EXCLUSIVE
)
