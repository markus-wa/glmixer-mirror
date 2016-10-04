cmake_minimum_required (VERSION 2.8)


if(APPLE)
    add_definitions(-DTARGET_OS_MAC)
endif(APPLE)

if(WIN32)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--kill-at")
endif(WIN32)

# multiplatform plugins
add_subdirectory(FreeFrameTestPlugin)
add_subdirectory(FreeFrameWavePlugin)
add_subdirectory(FreeFrameBlurPlugin)
add_subdirectory(FreeFrameMonteCarloPlugin)
add_subdirectory(FreeFrameFreiChenPlugin)
add_subdirectory(FreeFramePixelationPlugin)
add_subdirectory(FreeFrameDelayPlugin)
add_subdirectory(FreeFrameShadowPlugin)
add_subdirectory(FreeFrameShakePlugin)
add_subdirectory(FreeFrameQtScreenCapturePlugin)
add_subdirectory(FreeFrameFragmentCodePlugin)


# platform specific plugins & ressource
if(WIN32)

    add_subdirectory(ExtremelySimpleCaptureFFGLPlugin)

    if(USE_SPOUT)
        add_subdirectory(SpoutFFGLPlugin)
    endif(USE_SPOUT)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins_win32.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.qrc)

else(WIN32)

    if(APPLE)

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins_osx.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.qrc)

    else(APPLE)

# Linux
        add_subdirectory(video4LinuxFFGLPlugin)
        
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins_linux.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.qrc)
    endif(APPLE)

endif(WIN32)


