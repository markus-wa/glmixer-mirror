
cmake_minimum_required (VERSION 2.6)
project (LIBSHADERTOY)


SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBFFGLGLSL_SRCS
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp
    ${FREEFRAMEGL_DIR_COMMON}/FFGLFBO.cpp
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
)

if(WIN32)
  add_library(ShadertoyEffect SHARED ${LIBFFGLGLSL_SRCS} FreeFrameShadertoyEffect.cpp )
  add_library(ShadertoySource SHARED ${LIBFFGLGLSL_SRCS} FreeFrameShadertoySource.cpp )
else(WIN32)
  add_library(ShadertoyEffect MODULE ${LIBFFGLGLSL_SRCS} FreeFrameShadertoyEffect.cpp )
  add_library(ShadertoySource MODULE ${LIBFFGLGLSL_SRCS} FreeFrameShadertoySource.cpp )
endif(WIN32)

target_link_libraries(ShadertoyEffect ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
target_link_libraries(ShadertoySource ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

