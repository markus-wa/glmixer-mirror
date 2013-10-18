
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLWAVE)


SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBFFGLWAVE_SRCS
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp 
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
    FreeFrameWave.cpp
)

if(WIN32)
set(LIBFFGLWAVE_SRCS ${LIBFFGLWAVE_SRCS} FreeFrameWave.def)
endif(WIN32)

add_library (ffwave SHARED ${LIBFFGLWAVE_SRCS})

target_link_libraries(ffwave ${OPENGL_LIBRARY})
