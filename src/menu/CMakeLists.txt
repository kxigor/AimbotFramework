# =========================================================
# afw::menu  (STATIC)
# =========================================================
# The overlay's user interface, drawn on top of afw::config. A consumer of
# afw::render and a sibling of afw::esp -- the two never reference each other.
# Windows-only, so it is added from src/ only under WIN32.

add_library(afw_menu STATIC
  menu.cpp
)
add_library(afw::menu ALIAS afw_menu)

target_compile_features(afw_menu PUBLIC cxx_std_23)

target_include_directories(afw_menu
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_link_libraries(afw_menu PUBLIC afw::config)
target_link_libraries(afw_menu PRIVATE afw::render)

if(TARGET project_options)
  target_link_libraries(afw_menu PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_menu PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_menu PRIVATE project_coverage)
endif()
