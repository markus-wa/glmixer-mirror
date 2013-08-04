
cmake_minimum_required (VERSION 2.6)


# multiplatform plugins
add_subdirectory(FreeFrameTestPlugin)
add_subdirectory(FreeFrameDelayPlugin)


# platofm specific plugins
if(WIN32)

else(WIN32)

    if(APPLE)

    else(APPLE)
        add_subdirectory(FreeFrameQtPlugin)
        add_subdirectory(video4LinuxFFGLPlugin)
    endif(APPLE)

endif(WIN32)
