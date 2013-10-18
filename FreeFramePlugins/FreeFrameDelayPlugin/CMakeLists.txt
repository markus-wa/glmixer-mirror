
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLDELAY)


SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBFFGLDELAY_SRCS
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp 
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
    FreeFrameDelay.cpp
)

if(WIN32)
set(LIBFFGLDELAY_SRCS ${LIBFFGLDELAY_SRCS} FreeFrameDelay.def)
endif(WIN32)

add_library (ffdelay SHARED ${LIBFFGLDELAY_SRCS})

target_link_libraries(ffdelay ${OPENGL_LIBRARY} ${GLEW_LIBRARY}) 
