# =========================================================
# afw::render  (STATIC)
# =========================================================
# The overlay's foundation: owns the swap chain's Present hook, the graphics
# resources and the ImGui context, and offers its users a per-frame drawing
# callback. afw::menu and afw::esp are siblings on top of it, neither depending
# on the other. Windows-only, so it is added from src/ only under WIN32.
#
# Only the surface in include/afw/render/overlay.hpp is graphics-API neutral --
# a frame callback, the overlay window, the back buffer size. Everything that
# names IDXGISwapChain lives under dxgi/, in namespace afw::render::dxgi, and
# that is the whole of the implementation today. A title on another API (Doom 3
# is OpenGL, Q2RTX is Vulkan) needs a sibling directory implementing the same
# neutral surface; ImGui draw lists, and therefore afw::esp and afw::menu, are
# backend-independent and would not change.
#
# render/overlay.hpp carries no .cpp of its own on purpose: it states what a
# backend must provide, the way app/game.hpp and hook/hook.hpp state what a
# title and a hook must provide. Each source below does pair with the header
# beside it, so the layout rule holds -- dxgi/overlay.cpp answers both its own
# dxgi/overlay.hpp and the neutral surface.

add_library(afw_render STATIC
  dxgi/probe.cpp
  dxgi/present_hook.cpp
  dxgi/overlay.cpp
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

# D3D11CreateDeviceAndSwapChain lives in d3d11.dll; the probe is the only user.
target_link_libraries(afw_render PRIVATE d3d11 dxguid)

# Dear ImGui and its Win32/D3D11 backends, pinned as a submodule (third_party/).
target_link_libraries(afw_render PRIVATE imgui)

if(TARGET project_options)
  target_link_libraries(afw_render PRIVATE project_options)
endif()
if(TARGET project_sanitizers)
  target_link_libraries(afw_render PRIVATE project_sanitizers)
endif()
if(TARGET project_coverage)
  target_link_libraries(afw_render PRIVATE project_coverage)
endif()
