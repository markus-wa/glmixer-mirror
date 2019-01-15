
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLCLOCK)

if(WIN32)
  add_library(ffclock SHARED FreeFrameClock.cpp)
else(WIN32)
  add_library(ffclock MODULE FreeFrameClock.cpp)
endif(WIN32)


if(APPLE)
target_link_libraries(ffclock -Wl,-all_load FreeFrameGL ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
else(APPLE)
target_link_libraries(ffclock -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
endif(APPLE)
