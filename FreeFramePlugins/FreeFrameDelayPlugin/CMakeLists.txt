
cmake_minimum_required (VERSION 2.8)
project (LIBFFGLDELAY)

if(WIN32)
  add_library(ffglDelay SHARED FreeFrameDelay.cpp)
else(WIN32)
  add_library(ffglDelay MODULE FreeFrameDelay.cpp)
endif(WIN32)

target_link_libraries(ffglDelay -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive  ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

IF(CMAKE_VERSION VERSION_GREATER 2.9)
INSTALL(FILES $<TARGET_LINKER_FILE:ffglDelay>
        CONFIGURATIONS Release RelWithDebInfo
        RUNTIME DESTINATION "./lib/${GLMIXER_BINARY}" COMPONENT Runtime
)
ELSE()
    get_property(PLUGIN TARGET ffglDelay PROPERTY LOCATION)
    INSTALL(FILES ${PLUGIN}
            CONFIGURATIONS Release RelWithDebInfo
            RUNTIME DESTINATION "./lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ENDIF()
