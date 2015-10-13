
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLWAVE)

if(WIN32)
  add_library(ffglWave SHARED FreeFrameWave.cpp)
else(WIN32)
  add_library(ffglWave MODULE FreeFrameWave.cpp)
endif(WIN32)

target_link_libraries(ffglWave -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive   ${OPENGL_LIBRARY})

IF(CMAKE_VERSION VERSION_GREATER 2.9)
    INSTALL(FILES $<TARGET_LINKER_FILE:ffglWave>
        CONFIGURATIONS Release RelWithDebInfo
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ELSE()
    get_property(PLUGIN TARGET ffglWave PROPERTY LOCATION)
    INSTALL(FILES ${PLUGIN}
            CONFIGURATIONS Release RelWithDebInfo
            RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ENDIF()
