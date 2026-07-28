add_subdirectory(basic)

if(WIN32)
  add_subdirectory(win)
  add_subdirectory(remote)
  add_subdirectory(hook)
  add_subdirectory(render)
endif()
