
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLBLUR)


SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBFFGLBLUR_SRCS
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp
    ${FREEFRAMEGL_DIR_COMMON}/FFGLFBO.cpp
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
    FreeFrameBlur.cpp
)

if(WIN32)
  add_library(ffglBlur SHARED ${LIBFFGLBLUR_SRCS})
else(WIN32)
  add_library(ffglBlur MODULE ${LIBFFGLBLUR_SRCS})
endif(WIN32)

target_link_libraries(ffglBlur ${OPENGL_LIBRARY} ${GLEW_LIBRARY})


