# =========================================================
# afw::games::fc3  (STATIC)
# =========================================================
# Far Cry 3 (Dunia) specific bindings: offsets, structures, and the workflow to
# read entities / bones / view matrix. Builds on the generic afw::process.
# Skeleton for now. One target per game so you link only the title you target.

add_library(afw_games_fc3 STATIC
  module.cpp
  camera.cpp
  class_registry.cpp
  components.cpp
  entity.cpp
  agent.cpp
  pawn.cpp
  bones.cpp
  census.cpp
  snapshot.cpp
  error.cpp
  weapon.cpp
  trace.cpp
  reach.cpp
)
add_library(afw::games::fc3 ALIAS afw_games_fc3)

target_compile_features(afw_games_fc3 PUBLIC cxx_std_23)

target_include_directories(afw_games_fc3
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_link_libraries(afw_games_fc3 PUBLIC afw::process afw::hook afw::math)

if(TARGET project_options)
  target_link_libraries(afw_games_fc3 PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_games_fc3 PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_games_fc3 PRIVATE project_coverage)
endif()
