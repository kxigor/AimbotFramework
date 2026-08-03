# =========================================================
# afw::games::q2rtx  (STATIC)
# =========================================================
# Quake II RTX (Q2PRO lineage) specific bindings. The renderer is Vulkan, so
# this title is what forces the overlay to present through vkQueuePresentKHR
# rather than a swap chain vtable. Single-player only and open source, so every
# finding is checkable against real code.
# Skeleton for now. One target per game so you link only the title you target.

add_library(afw_games_q2rtx STATIC
  q2rtx.cpp
)
add_library(afw::games::q2rtx ALIAS afw_games_q2rtx)

target_compile_features(afw_games_q2rtx PUBLIC cxx_std_23)

target_include_directories(afw_games_q2rtx
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_link_libraries(afw_games_q2rtx PUBLIC afw::process afw::hook afw::math)

if(TARGET project_options)
  target_link_libraries(afw_games_q2rtx PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_games_q2rtx PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_games_q2rtx PRIVATE project_coverage)
endif()
