
cmake_minimum_required (VERSION 2.6)


# multiplatform plugins
add_subdirectory(FreeFrameTestPlugin)
add_subdirectory(FreeFrameWavePlugin)
add_subdirectory(FreeFrameBlurPlugin)
add_subdirectory(FreeFrameDelayPlugin)
add_subdirectory(FreeFrameQtScreenCapturePlugin)
add_subdirectory(FreeFrameQtFragmentCodePlugin)


# platofm specific plugins
if(WIN32)

    add_subdirectory(ExtremelySimpleCaptureFFGLPlugin)

else(WIN32)

    if(APPLE)

    else(APPLE)
        add_subdirectory(FreeFrameQtPlugin)
        add_subdirectory(video4LinuxFFGLPlugin)
    endif(APPLE)

endif(WIN32)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plugins.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.qrc)

