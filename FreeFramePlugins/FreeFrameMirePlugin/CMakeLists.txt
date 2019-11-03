
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLCMIRE)

if(WIN32)
  add_library(ffmire SHARED FreeFrameMire.cpp)
else(WIN32)
  add_library(ffmire MODULE FreeFrameMire.cpp)
endif(WIN32)


if(APPLE)
target_link_libraries(ffmire -Wl,-all_load FreeFrameGL ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
else(APPLE)
target_link_libraries(ffmire -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
endif(APPLE)
