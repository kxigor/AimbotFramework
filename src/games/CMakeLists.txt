# =========================================================
# GAME-SPECIFIC LAYER  (afw::games::*)
# =========================================================
# One subdirectory per supported title. Each is a separate static library so a
# build links only the game it targets. The three titles are deliberately on
# three different renderers -- Direct3D 11, OpenGL and Vulkan -- so the overlay
# cannot quietly stay tied to one of them.

add_subdirectory(fc3)
add_subdirectory(doom3)
add_subdirectory(q2rtx)
