
cmake_minimum_required (VERSION 2.6)


# inherited from above
#find_path( FREEFRAMEGL_INCLUDE "FFGL.h" HINTS "${FREEFRAMEGL_PATH}/Include" )
#include_directories(${FREEFRAMEGL_INCLUDE})


# multiplatform plugins
add_subdirectory(FreeFrameTestPlugin)


# platofm specific plugins
if(WIN32)

else(WIN32)

    if(APPLE)

    else(APPLE)
        add_subdirectory(FreeFrameQtPlugin)
        add_subdirectory(video4LinuxFFGLPlugin)
    endif(APPLE)

endif(WIN32)
