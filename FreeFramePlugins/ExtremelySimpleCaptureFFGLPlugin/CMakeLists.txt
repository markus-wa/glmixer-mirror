
cmake_minimum_required (VERSION 2.6)
project (LIBESCAPIFFGL)


SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBESCAPIFFGL_SRCS
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp 
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
    escapi.cpp
    escapiFFGL.cpp
    escapiFFGL.def
)

find_library( PTHREAD_LIBRARY pthread REQUIRED)

add_library (escapiffgl SHARED ${LIBESCAPIFFGL_SRCS} )

target_link_libraries(escapiffgl ${OPENGL_LIBRARY} ${PTHREAD_LIBRARY} ${GLEW_LIBRARY})


