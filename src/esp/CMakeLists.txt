# =========================================================
# afw::esp  (STATIC)
# =========================================================
# Projects entities read through afw::games::fc3 into screen space and draws
# them over the frame. A consumer of afw::render and a sibling of afw::menu, so
# it runs with settings alone and needs no user interface present. Windows-only,
# so it is added from src/ only under WIN32.

add_library(afw_esp STATIC
  projection.cpp
  classify.cpp
  draw.cpp
  panel_world.cpp
  panel_entities.cpp
  panel_pools.cpp
  esp.cpp
)
add_library(afw::esp ALIAS afw_esp)

target_compile_features(afw_esp PUBLIC cxx_std_23)

target_include_directories(afw_esp
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_link_libraries(afw_esp PUBLIC afw::config afw::math)
target_link_libraries(afw_esp PRIVATE afw::render afw::games::fc3 imgui)

if(TARGET project_options)
  target_link_libraries(afw_esp PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_esp PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_esp PRIVATE project_coverage)
endif()
