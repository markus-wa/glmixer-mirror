
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLSHADOW)

if(WIN32)
  add_library(ffShadow SHARED FreeFrameShadow.cpp)
else(WIN32)
  add_library(ffShadow MODULE FreeFrameShadow.cpp)
endif(WIN32)

target_link_libraries(ffShadow -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${OPENGL_LIBRARY})

IF(CMAKE_VERSION VERSION_GREATER 2.8)
    INSTALL(FILES $<TARGET_LINKER_FILE:ffShadow>
        CONFIGURATIONS Release RelWithDebInfo
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ELSE()
    get_property(PLUGIN TARGET ffShadow PROPERTY LOCATION)
    INSTALL(FILES ${PLUGIN}
            CONFIGURATIONS Release RelWithDebInfo
            RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/${GLMIXER_BINARY}" COMPONENT Runtime
    )
ENDIF()
