
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLDELAY)

if(WIN32)
  add_library(ffglDelay SHARED FreeFrameDelay.cpp)
else(WIN32)
  add_library(ffglDelay MODULE FreeFrameDelay.cpp)
endif(WIN32)

target_link_libraries(ffglDelay -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive  ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

get_property(PLUGIN TARGET ffglDelay PROPERTY LOCATION)
INSTALL(FILES ${PLUGIN} DESTINATION plugins COMPONENT Runtime)
