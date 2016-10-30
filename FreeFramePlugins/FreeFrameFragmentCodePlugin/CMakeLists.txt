
cmake_minimum_required (VERSION 2.8)
project (LIBSHADERTOY)

if(WIN32)
  add_library(ShadertoyEffect SHARED FreeFrameShadertoyEffect.cpp )
  add_library(ShadertoySource SHARED FreeFrameShadertoySource.cpp )
else(WIN32)
  add_library(ShadertoyEffect MODULE FreeFrameShadertoyEffect.cpp )
  add_library(ShadertoySource MODULE FreeFrameShadertoySource.cpp )
endif(WIN32)

message( STATUS "Using ShadertoyEffect and ShadertoySource")

if(APPLE)
target_link_libraries(ShadertoyEffect FreeFrameGL ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
target_link_libraries(ShadertoySource FreeFrameGL ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
else(APPLE)
target_link_libraries(ShadertoyEffect -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
target_link_libraries(ShadertoySource -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
endif(APPLE)
