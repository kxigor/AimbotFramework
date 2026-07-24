# =========================================================
# afw::aimbot  (STATIC)
# =========================================================
# The aim-assist logic. Depends on afw::process for reading game state.
# Skeleton for now.

add_library(afw_aimbot STATIC
  aimbot.cpp   # replace with your real sources
)
add_library(afw::aimbot ALIAS afw_aimbot)

target_compile_features(afw_aimbot PUBLIC cxx_std_23)

target_include_directories(afw_aimbot
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# aimbot builds on top of the process-access library.
target_link_libraries(afw_aimbot PUBLIC afw::process)

if(TARGET project_options)
  target_link_libraries(afw_aimbot PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_aimbot PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_aimbot PRIVATE project_coverage)
endif()
