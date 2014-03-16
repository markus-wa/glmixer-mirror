
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLQTGLSL)

set(LIBFFGLQTGLSL_SRCS
    FreeFrameFragmentCodePlugin.cpp
)

SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBFFGLQTGLSL_SRCS
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp 
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp 
    ${FREEFRAMEGL_DIR_COMMON}/FFGLFBO.cpp
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
    ${LIBFFGLQTGLSL_SRCS}
)

if(WIN32)
  add_library(FreeFrameFragmentCodePlugin SHARED ${LIBFFGLQTGLSL_SRCS})
else(WIN32)
  add_library(FreeFrameFragmentCodePlugin MODULE ${LIBFFGLQTGLSL_SRCS})
endif(WIN32)

target_link_libraries(FreeFrameFragmentCodePlugin ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

