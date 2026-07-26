# =========================================================
# afw::config  (STATIC)
# =========================================================
# Settings shared by afw::menu and afw::esp: feature toggles, colours, distance
# limits. It exists so neither of those two has to own the other's state. Plain
# data with no Win32 dependency, so it is built on every platform.

add_library(afw_config STATIC
  config.cpp
)
add_library(afw::config ALIAS afw_config)

target_compile_features(afw_config PUBLIC cxx_std_23)

target_include_directories(afw_config
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

if(TARGET project_options)
  target_link_libraries(afw_config PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_config PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_config PRIVATE project_coverage)
endif()
