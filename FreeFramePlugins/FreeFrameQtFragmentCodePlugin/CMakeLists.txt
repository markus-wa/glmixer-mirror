
cmake_minimum_required (VERSION 2.6)
project (LIBFFGLQTGLSL)

# add the corresponding path to include
include(${QT_USE_FILE})
IF (WIN32)
    SET (QT_USE_QTMAIN TRUE)
ENDIF (WIN32)

set(LIBFFGLQTGLSL_SRCS
    FreeFrameQtGLSL.cpp
    GLSLCodeEditor.cpp
    GLSLCodeEditorWidget.cpp
    GLSLSyntaxHighlighter.cpp
)

# do the qt-cmake magic
qt4_wrap_ui(LIBFFGLQTGLSL_UIS_H CodeEditorWidget.ui)

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( ${LIBFFGLQTGLSL_UIS_H} ${LIBFFGLQTGLSL_SRCS} )


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
    ${LIBFFGLQTGLSL_UIS_H}
)

if(WIN32)
  add_library(ffglQtFragmentProgram SHARED ${LIBFFGLQTGLSL_SRCS})
else(WIN32)
  add_library(ffglQtFragmentProgram MODULE ${LIBFFGLQTGLSL_SRCS})
endif(WIN32)

target_link_libraries(ffglQtFragmentProgram ${OPENGL_LIBRARY} ${QT_LIBRARIES}  ${GLEW_LIBRARY})


