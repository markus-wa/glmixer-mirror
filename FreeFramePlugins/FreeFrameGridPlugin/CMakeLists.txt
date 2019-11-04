
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLGRID)

if(WIN32)
  add_library(ffgrid SHARED FreeFrameGrid.cpp)
else(WIN32)
  add_library(ffgrid MODULE FreeFrameGrid.cpp)
endif(WIN32)


if(APPLE)
target_link_libraries(ffgrid -Wl,-all_load FreeFrameGL ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
else(APPLE)
target_link_libraries(ffgrid -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
endif(APPLE)
