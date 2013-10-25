
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLQTGLSL)

# add the corresponding path to include
include(${QT_USE_FILE})
IF (WIN32)
    SET (QT_USE_QTMAIN TRUE)
ENDIF (WIN32)

set(LIBFFGLQTTEST_SRCS
    FreeFrameQtGLSL.cpp
    GLSLCodeEditorWidget.cpp
)
# do the qt-cmake magic
qt4_wrap_ui(LIBFFGLQTGLSL_UIS_SRC GLSLCodeEditorWidget.ui)

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( ${LIBFFGLQTGLSL_UIS_SRC} ${LIBFFGLQTGLSL_SRCS} )

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})


SET(FREEFRAMEGL_DIR_PLUGINS "${FREEFRAMEGL_PATH}/Source/FFGLPlugins/")
SET(FREEFRAMEGL_DIR_COMMON "${FREEFRAMEGL_PATH}/Source/Common/")

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBFFGLQTGLSL_SRCS
    ${LIBFFGLQTTEST_SRCS}
    ${FREEFRAMEGL_DIR_COMMON}/FFGLShader.cpp 
    ${FREEFRAMEGL_DIR_COMMON}/FFGLExtensions.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfo.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginInfoData.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGL.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginManager.cpp 
    ${FREEFRAMEGL_DIR_PLUGINS}/FFGLPluginSDK.cpp
)

add_library (ffglqtglsl SHARED ${LIBFFGLQTGLSL_SRCS} ${LIBFFGLQTGLSL_UIS_SRC})

