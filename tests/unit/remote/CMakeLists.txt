add_gtest(RemoteProcUnit
  SOURCES FindProcessByImageNameUnit.cpp
  LIBRARIES afw::remote afw::win project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 30
)

add_gtest(RemoteInjectUnit
  SOURCES InjectLibraryUnit.cpp
  LIBRARIES afw::remote afw::win project_options project_sanitizers project_coverage
  INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/tests/unit
  PROPERTIES TIMEOUT 60 LABELS injection
  EXCLUSIVE
)
