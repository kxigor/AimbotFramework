# =========================================================
# afw::math  (STATIC)
# =========================================================
# Vector and matrix types shared by everything that does geometry. Vec3 is also
# the wire format the engine's structures are read into, which is why it stays a
# trivially copyable aggregate (see include/afw/math/vec3.hpp). No Win32
# dependency, so it is built on every platform.

add_library(afw_math STATIC
  vec3.cpp
)
add_library(afw::math ALIAS afw_math)

target_compile_features(afw_math PUBLIC cxx_std_23)

target_include_directories(afw_math
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

if(TARGET project_options)
  target_link_libraries(afw_math PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_math PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_math PRIVATE project_coverage)
endif()
