cmake_minimum_required (VERSION 2.8)
project (LIBFFGLMONTECARLO)

if(WIN32)
  add_library(ffglMonteCarlo SHARED FreeFrameMonteCarlo.cpp)
else(WIN32)
  add_library(ffglMonteCarlo MODULE FreeFrameMonteCarlo.cpp)
endif(WIN32)

target_link_libraries(ffglMonteCarlo -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

IF(CMAKE_VERSION VERSION_GREATER 2.9)
    INSTALL(FILES $<TARGET_LINKER_FILE:ffglMonteCarlo>
        CONFIGURATIONS Release RelWithDebInfo
        RUNTIME DESTINATION "./lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ELSE()
    get_property(PLUGIN TARGET ffglMonteCarlo PROPERTY LOCATION)
    INSTALL(FILES ${PLUGIN}
            CONFIGURATIONS Release RelWithDebInfo
            RUNTIME DESTINATION "./lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ENDIF()
