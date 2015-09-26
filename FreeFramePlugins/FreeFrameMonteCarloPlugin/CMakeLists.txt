cmake_minimum_required (VERSION 2.6)
project (LIBFFGLMONTECARLO)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

if(WIN32)
  add_library(ffglMonteCarlo SHARED FreeFrameMonteCarlo.cpp)
else(WIN32)
  add_library(ffglMonteCarlo MODULE FreeFrameMonteCarlo.cpp)
endif(WIN32)

target_link_libraries(ffglMonteCarlo -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

get_property(PLUGIN TARGET ffglMonteCarlo PROPERTY LOCATION)
INSTALL(FILES ${PLUGIN} DESTINATION lib/glmixer COMPONENT Runtime)
