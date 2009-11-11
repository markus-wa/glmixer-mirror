# the next line sets up include and link directories and defines some variables that we will use.
# you can modify the behavior by setting some variables, e.g.
set(QT_USE_OPENGL TRUE)
# -> this will cause cmake to include and link against the OpenGL module
include(${QT_USE_FILE})

# the variable "qtproject_SRCS" contains all .cpp files of this project
set(MYQT_SRCS
    VideoFileDialogPreview.cpp
    VideoFileDialog.cpp
    VideoFileDisplayWidget.cpp
    VideoFile.cpp
    VideoSource.cpp
	glRenderWidget.cpp
    MainRenderWidget.cpp
    MixerViewWidget.cpp
    glmixer.cpp
    main.cpp
)

set(MYQT_UIS
    glmixer.ui
    VideoFileDialog.ui
)

set(MYQT_RCS
    ../icons.qrc
)

qt4_wrap_ui(MYQT_UIS_H ${MYQT_UIS})

qt4_add_resources(MYQT_RCS_SRCS ${MYQT_RCS})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# this application uses qt libs
include_directories(${QT_QTOPENGL_INCLUDE_DIR})

# this application uses openGL
include_directories(${OPENGL_INCLUDE_DIR})

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( ${MYQT_UIS_H} ${MYQT_SRCS})

# create an executable file named "qtproject" from the source files in the variable "qtproject_SRCS".
add_executable(glmixer ${MYQT_SRCS} ${MYQT_UIS_H} ${MYQT_RCS_SRCS} )

# link the "qtproject" target against the Qt libraries. which libraries exactly, is defined by the "include(${QT_USE_FILE})" line above, which sets up this variable.
target_link_libraries(glmixer ${QT_LIBRARIES} ${QT_QTOPENGL_LIBRARIES} ${OPENGL_LIBRARY} ${FFMPEG_LIBRARIES})

install(TARGETS glmixer DESTINATION bin)


 
