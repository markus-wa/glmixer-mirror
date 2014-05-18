
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLTEST)

if(WIN32)
  add_library(fftest SHARED FreeFrameTest.cpp)
else(WIN32)
  add_library(fftest MODULE FreeFrameTest.cpp)
endif(WIN32)

target_link_libraries(fftest -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY})
