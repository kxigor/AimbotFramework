add_gtest(WinUnit
  SOURCES
    ScopedHandleUnit.cpp
    RemoteMemoryUnit.cpp
    ProtectGuardUnit.cpp
    LastErrorUnit.cpp
  LIBRARIES afw::win project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)
