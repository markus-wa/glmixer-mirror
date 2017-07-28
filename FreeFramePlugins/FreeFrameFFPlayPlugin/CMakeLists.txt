
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLFFPLAY)

if(WIN32)
  add_library(ffffplay SHARED FreeFrameFFPlay.cpp)
else(WIN32)
  add_library(ffffplay MODULE FreeFrameFFPlay.cpp)
endif(WIN32)


if(APPLE)
  target_link_libraries(ffffplay -Wl,-all_load FreeFrameGL ${OPENGL_LIBRARY})
else(APPLE)
  target_link_libraries(ffffplay -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY})
endif(APPLE)
