# =========================================================
# afw::aimbot  (STATIC)
# =========================================================
# Where to aim and at whom. Pure geometry over an already-taken snapshot: it
# reads no memory of its own and holds no opinion about who is hostile, so the
# whole of it is testable off the game.

add_library(afw_aimbot STATIC
  aimbot.cpp
)
add_library(afw::aimbot ALIAS afw_aimbot)

target_compile_features(afw_aimbot PUBLIC cxx_std_23)

target_include_directories(afw_aimbot
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# The snapshot it scores comes from the game binding, and the geometry from
# afw::math. Reading the game is somebody else's job.
target_link_libraries(afw_aimbot PUBLIC afw::math afw::games::fc3)

if(TARGET project_options)
  target_link_libraries(afw_aimbot PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_aimbot PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_aimbot PRIVATE project_coverage)
endif()
