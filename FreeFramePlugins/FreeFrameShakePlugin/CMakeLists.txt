
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLSHAKE)

if(WIN32)
  add_library(ffglShake SHARED FreeFrameShake.cpp)
else(WIN32)
  add_library(ffglShake MODULE FreeFrameShake.cpp)
endif(WIN32)

target_link_libraries(ffglShake -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY})

IF(CMAKE_VERSION VERSION_GREATER 2.9)
    INSTALL(FILES $<TARGET_FILE:ffglShake>
        CONFIGURATIONS Release RelWithDebInfo
        RUNTIME DESTINATION "./lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )

ELSE()
    get_property(PLUGIN TARGET ffglShake PROPERTY LOCATION)
    INSTALL(FILES ${PLUGIN}
            CONFIGURATIONS Release RelWithDebInfo
            RUNTIME DESTINATION "./lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ENDIF()
