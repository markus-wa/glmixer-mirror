
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLSHADOW)

if(WIN32)
  add_library(ffShadow SHARED FreeFrameShadow.cpp)
else(WIN32)
  add_library(ffShadow MODULE FreeFrameShadow.cpp)
endif(WIN32)

target_link_libraries(ffShadow -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY})

get_property(PLUGIN TARGET ffShadow PROPERTY LOCATION)
INSTALL(FILES ${PLUGIN} DESTINATION lib/glmixer COMPONENT Runtime)
