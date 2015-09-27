cmake_minimum_required (VERSION 2.8)
project (LIBFFGLMONTECARLO)

if(WIN32)
  add_library(ffglMonteCarlo SHARED FreeFrameMonteCarlo.cpp)
else(WIN32)
  add_library(ffglMonteCarlo MODULE FreeFrameMonteCarlo.cpp)
endif(WIN32)

target_link_libraries(ffglMonteCarlo -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

INSTALL(FILES $<TARGET_LINKER_FILE:ffglMonteCarlo>
        CONFIGURATIONS Release RelWithDebInfo
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${GLMIXER_BINARY}" COMPONENT Runtime
)
