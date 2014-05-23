
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLWAVE)

if(WIN32)
  add_library(ffglWave SHARED FreeFrameWave.cpp)
else(WIN32)
  add_library(ffglWave MODULE FreeFrameWave.cpp)
endif(WIN32)

target_link_libraries(ffglWave -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive   ${OPENGL_LIBRARY})

get_property(PLUGIN TARGET ffglWave PROPERTY LOCATION)
INSTALL(FILES ${PLUGIN} DESTINATION plugins COMPONENT Runtime)
