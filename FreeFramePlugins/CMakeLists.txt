
cmake_minimum_required (VERSION 2.6)


# multiplatform plugins
add_subdirectory(FreeFrameTestPlugin)
add_subdirectory(FreeFrameWavePlugin)
add_subdirectory(FreeFrameBlurPlugin)
add_subdirectory(FreeFrameDelayPlugin)
add_subdirectory(FreeFrameQtScreenCapturePlugin)
add_subdirectory(FreeFrameFragmentCodePlugin)


# platform specific plugins & ressource
if(WIN32)

    add_subdirectory(ExtremelySimpleCaptureFFGLPlugin)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins_win32.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.qrc)

else(WIN32)

    if(APPLE)

    else(APPLE)
        # add_subdirectory(FreeFrameQtPlugin)
        add_subdirectory(video4LinuxFFGLPlugin)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins_linux.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.qrc)
    endif(APPLE)

endif(WIN32)


