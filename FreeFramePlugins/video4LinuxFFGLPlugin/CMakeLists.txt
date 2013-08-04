
cmake_minimum_required (VERSION 2.6)
project (LIBV4LFFGL)


SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBV4LFFGL_SRCS
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp 
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
    v4lFFGL.cpp
)

find_library( V4L2_LIBRARY NAMES v4l2)
find_library( V4L_CONVERT_LIBRARY NAMES v4lconvert)
find_library( PTHREAD_LIBRARY pthread)

add_library (v4lffgl SHARED ${LIBV4LFFGL_SRCS})

target_link_libraries(v4lffgl  ${V4L2_LIBRARY} ${V4L_CONVERT_LIBRARY} ${PTHREAD_LIBRARY})


