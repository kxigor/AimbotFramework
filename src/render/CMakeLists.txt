# =========================================================
# afw::render  (STATIC)
# =========================================================
# The overlay's foundation: owns the IDXGISwapChain::Present hook, the Direct3D
# 11 resources and the ImGui context, and offers its users a per-frame drawing
# callback. afw::menu and afw::esp are siblings on top of it, neither depending
# on the other. Windows-only, so it is added from src/ only under WIN32.

add_library(afw_render STATIC
  render.cpp
)
add_library(afw::render ALIAS afw_render)

target_compile_features(afw_render PUBLIC cxx_std_23)

target_include_directories(afw_render
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# The hook objects are held by value in render's own headers, so afw::hook is
# part of its interface rather than an implementation detail.
target_link_libraries(afw_render PUBLIC afw::hook)
target_link_libraries(afw_render PRIVATE afw::win)

if(TARGET project_options)
  target_link_libraries(afw_render PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_render PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_render PRIVATE project_coverage)
endif()
