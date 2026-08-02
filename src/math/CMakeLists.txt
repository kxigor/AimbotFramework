# =========================================================
# afw::math  (INTERFACE)
# =========================================================
# Vec3, Aabb and Matrix4. Header-only: every one of these is a handful of float
# operations on the path each entity takes each frame, and there is no LTO in
# this build, so an out-of-line definition could never be inlined into its
# callers. They are also the wire format the engine's structures are read into,
# which is why they stay trivially copyable aggregates.

add_library(afw_math INTERFACE)
add_library(afw::math ALIAS afw_math)

target_compile_features(afw_math INTERFACE cxx_std_23)

target_include_directories(afw_math
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
