# =========================================================
# DLL SHIMS
# =========================================================
# One injectable library per title. Each subdirectory owns a single game: the
# afw::app::Game implementation, the per-title glue that decides who counts and
# where to point, and its own DllMain. Nothing here is shared between titles --
# what is shared lives in the libraries under src/.
#
# A title is added by adding a directory beside fc3/, not by teaching this one
# about a second game.

add_subdirectory(fc3)
