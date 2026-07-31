# =========================================================
# imgui  (STATIC, git submodule)
# =========================================================
# Dear ImGui with the Win32 and Direct3D 11 backends. The sources live in the
# submodule pinned at third_party/imgui (see .gitmodules); nothing in there is
# ours, so the target is declared from here instead of from a CMakeLists placed
# inside someone else's repository.
#
# It deliberately does not link project_options: that warning set is tuned for
# our own code and -Werror would fail on sources we do not own. For the same
# reason the include directories are SYSTEM, so warnings from imgui.h do not
# surface in the translation units that include it. FormatCheck and TidyCheck
# never see any of this -- their globs only reach src/, include/ and dll/.

set(AFW_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

if(NOT EXISTS "${AFW_IMGUI_DIR}/imgui.cpp")
  message(FATAL_ERROR
    "third_party/imgui is empty: the submodule has not been checked out.\n"
    "  git submodule update --init --recursive")
endif()

add_library(imgui STATIC
  ${AFW_IMGUI_DIR}/imgui.cpp
  ${AFW_IMGUI_DIR}/imgui_draw.cpp
  ${AFW_IMGUI_DIR}/imgui_tables.cpp
  ${AFW_IMGUI_DIR}/imgui_widgets.cpp
  ${AFW_IMGUI_DIR}/imgui_demo.cpp
  ${AFW_IMGUI_DIR}/backends/imgui_impl_win32.cpp
  ${AFW_IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)
add_library(afw::imgui ALIAS imgui)

target_compile_features(imgui PUBLIC cxx_std_23)

target_include_directories(imgui SYSTEM PUBLIC
  $<BUILD_INTERFACE:${AFW_IMGUI_DIR}>
  $<BUILD_INTERFACE:${AFW_IMGUI_DIR}/backends>
)

# The Win32 backend polls gamepads through XInput. The overlay has no use for
# one, and the guard also drops the header and the runtime LoadLibrary probe.
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

# d3dcompiler: the D3D11 backend compiles its vertex and pixel shaders at run
# time. dwmapi: the Win32 backend queries the compositor for frame timing.
target_link_libraries(imgui PUBLIC d3d11 d3dcompiler dwmapi)
