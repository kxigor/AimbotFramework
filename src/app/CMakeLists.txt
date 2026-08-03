# =========================================================
# afw::app  (STATIC)
# =========================================================
# The injected DLL's session, with nothing title-specific in it: the file log,
# the Present and window-procedure hooks, the overlay's menu, the detour drain
# and the self-unload. A game plugs in through afw::app::Game -- its own hooks
# and its own per-frame drawing -- so this layer is what a second title reuses
# unchanged. Windows-only, so it is added from src/ only under WIN32.

add_library(afw_app STATIC
  app.cpp
  detour_guard.cpp
  log.cpp
  module.cpp
)
add_library(afw::app ALIAS afw_app)

target_compile_features(afw_app PUBLIC cxx_std_23)

target_include_directories(afw_app
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# A game builds its own hooks with afw::hook and hands them to ScopedHooks, so
# the hook library is part of what afw::app's users compile against.
target_link_libraries(afw_app PUBLIC afw::hook)
target_link_libraries(afw_app PRIVATE afw::render afw::menu afw::config afw::win)

if(TARGET project_options)
  target_link_libraries(afw_app PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_app PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_app PRIVATE project_coverage)
endif()
