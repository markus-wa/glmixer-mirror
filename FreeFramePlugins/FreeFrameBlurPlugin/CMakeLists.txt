
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLBLUR)

if(WIN32)
  add_library(ffglBlur SHARED FreeFrameBlur.cpp)
else(WIN32)
  add_library(ffglBlur MODULE FreeFrameBlur.cpp)
endif(WIN32)

target_link_libraries(ffglBlur -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})


