
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLSHAKE)

if(WIN32)
  add_library(ffglShake SHARED FreeFrameShake.cpp)
else(WIN32)
  add_library(ffglShake MODULE FreeFrameShake.cpp)
endif(WIN32)

target_link_libraries(ffglShake -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY})
