
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLBLUR)

if(WIN32)
  add_library(ffglBlur SHARED FreeFrameBlur.cpp)
else(WIN32)
  add_library(ffglBlur MODULE FreeFrameBlur.cpp)
endif(WIN32)

target_link_libraries(ffglBlur -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

INSTALL(FILES $<TARGET_LINKER_FILE:ffglBlur>
        CONFIGURATIONS Release RelWithDebInfo
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${GLMIXER_BINARY}" COMPONENT Runtime
)
