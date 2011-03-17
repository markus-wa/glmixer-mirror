# the next line sets up include and link directories and defines some variables that we will use.

# this application uses QT with the following modules:
set(QT_USE_OPENGL    TRUE)
set(QT_USE_QTOPENGL  TRUE)
set(QT_USE_QTXML     TRUE)
set(QT_USE_QTMAIN    TRUE)
# add the corresponding path to include
include(${QT_USE_FILE})

# the variable "qtproject_SRCS" contains all .cpp files of this project
set(MYQT_SRCS
    MixSource.cpp
    glRenderWidget.cpp
    CameraDialog.cpp
    VideoFileDialogPreview.cpp
    VideoFileDialog.cpp
    VideoFileDisplayWidget.cpp
    VideoFile.cpp
    Source.cpp
    VideoSource.cpp
    AlgorithmSource.cpp
    AlgorithmSelectionDialog.cpp
    SourceDisplayWidget.cpp
    MixerView.cpp
    GeometryView.cpp
    LayersView.cpp
    CatalogView.cpp
    ViewRenderWidget.cpp
    RenderingManager.cpp
    RenderingEncoder.cpp
    OutputRenderWindow.cpp
    SourcePropertyBrowser.cpp
    SpringCursor.cpp
    DelayCursor.cpp
    MagnetCursor.cpp
    UserPreferencesDialog.cpp
    GammaLevelsWidget.cpp
    SessionSwitcherWidget.cpp
    glmixer.cpp
    main.cpp
)

set(MYQT_RCS
    ../icons.qrc
    ../textures.qrc
    ../images.qrc
)

set(MYQT_UIS
    glmixer.ui
    VideoFileDialog.ui
    CameraDialog.ui
    AlgorithmSelectionDialog.ui
    UserPreferencesDialog.ui
    GammaLevelsDialog.ui
)


# If there is OpenCV, then this application will use OpenCV
if (OpenCV_FOUND)
  include_directories(${OpenCV_INCLUDE_DIR})
  add_definitions(-DOPEN_CV )
  set( MYQT_SRCS  ${MYQT_SRCS} OpencvSource.cpp )
endif (OpenCV_FOUND)

# do the qt-cmake magic
qt4_wrap_ui(MYQT_UIS_H ${MYQT_UIS})
qt4_add_resources(MYQT_RCS_SRCS ${MYQT_RCS})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# this application uses the library QtProperty
include_directories(${LIBQTPROPERTY_SOURCE_DIR} )
link_directories (${LIBQTPROPERTY_BINARY_DIR}) 

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( ${MYQT_UIS_H} ${MYQT_SRCS})

if( APPLE )

SET(MACOSX_BUNDLE_INFO_STRING "GLMixer Bundle")
SET(MACOSX_BUNDLE_ICON_FILE "glmixer.icns")
SET(MACOSX_BUNDLE_LONG_VERSION_STRING "version 0.6")
SET(MACOSX_BUNDLE_BUNDLE_NAME "GLMixer")
SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "0.6")
SET(MACOSX_BUNDLE_BUNDLE_VERSION "0.6")
SET(MACOSX_BUNDLE_COPYRIGHT "Bruno Herbelin")

# create a BUNDLE
add_executable(glmixer MACOSX_BUNDLE ${MYQT_SRCS} ${MYQT_UIS_H} ${MYQT_RCS_SRCS} )

# link the "qtproject" target against the Qt libraries. which libraries exactly, is defined by the "include(${QT_USE_FILE})" line above, which sets up this variable.
target_link_libraries(glmixer QtProperty ${QT_LIBRARIES}  ${GLMIXER_LIBRARIES} )

set(GLMIXER_APP_BUNDLE_ROOT "${CMAKE_BINARY_DIR}/src/glmixer.app/") 

add_custom_target(bundle)

# Qt FRAMEWORKS
glmixer_install_qt4_framework(QtCore
  TARGET bundle NAME QtCore
  LIBRARY ${QT_QTCORE_LIBRARY_RELEASE}/QtCore
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
)
glmixer_install_qt4_framework(QtGui
  TARGET bundle NAME QtGui
  LIBRARY ${QT_QTGUI_LIBRARY_RELEASE}/QtGui
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtCore}
)
glmixer_install_qt4_framework(QtOpenGL
  TARGET bundle NAME QtOpenGL
  LIBRARY ${QT_QTOPENGL_LIBRARY_RELEASE}/QtOpenGL
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)
glmixer_install_qt4_framework(QtXml
  TARGET bundle NAME QtXml
  LIBRARY ${QT_QTXML_LIBRARY_RELEASE}/QtXml
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtCore}
)
glmixer_install_qt4_framework(QtSvg
  TARGET bundle NAME QtSvg
  LIBRARY ${QT_QTSVG_LIBRARY_RELEASE}/QtSvg
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)

# QT Plugins
GLMIXER_INSTALL_DYLIB(libqjpeg.dylib
  TARGET bundle NAME libqjpeg.dylib
  LIBRARY /Developer/Applications/Qt/plugins/imageformats/libqjpeg.dylib
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)
GLMIXER_INSTALL_DYLIB(libqico.dylib
  TARGET bundle NAME libqico.dylib
  LIBRARY /Developer/Applications/Qt/plugins/imageformats/libqico.dylib
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)
GLMIXER_INSTALL_DYLIB(libqgif.dylib
  TARGET bundle NAME libqgif.dylib
  LIBRARY /Developer/Applications/Qt/plugins/imageformats/libqgif.dylib
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)
GLMIXER_INSTALL_DYLIB(libqmng.dylib
  TARGET bundle NAME libqmng.dylib
  LIBRARY /Developer/Applications/Qt/plugins/imageformats/libqmng.dylib
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)
GLMIXER_INSTALL_DYLIB(libqsvg.dylib
  TARGET bundle NAME libqsvg.dylib
  LIBRARY /Developer/Applications/Qt/plugins/imageformats/libqsvg.dylib
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)
GLMIXER_INSTALL_DYLIB(libqtiff.dylib
  TARGET bundle NAME libqtiff.dylib
  LIBRARY /Developer/Applications/Qt/plugins/imageformats/libqtiff.dylib
  APP_BUNDLE ${GLMIXER_APP_BUNDLE_ROOT}
  DEPENDS_FRAMEWORKS ${QtGui} ${QtCore}
)


set(GLMIXER_APP_BUNDLE_EXECUTABLE "${CMAKE_BINARY_DIR}/src/glmixer.app/Contents/MacOS/glmixer") 

if (OpenCV_FOUND)
	glmixer_install_name_tool(${GLMIXER_APP_BUNDLE_EXECUTABLE}
	  TARGET bundle
	  FRAMEWORKS ${QtCore} ${QtGui} ${QtOpenGL} ${QtXml} ${QtSvg} 
	  LIBRARIES libqjpeg.dylib libqico.dylib libqgif.dylib libqmng.dylib libqsvg.dylib libqtiff.dylib 
	)
else (OpenCV_FOUND)
	glmixer_install_name_tool(${GLMIXER_APP_BUNDLE_EXECUTABLE}
	  TARGET bundle
	  FRAMEWORKS ${QtCore} ${QtGui} ${QtOpenGL} ${QtXml} ${QtSvg}
	  LIBRARIES libqjpeg.dylib libqico.dylib libqgif.dylib libqmng.dylib libqsvg.dylib libqtiff.dylib
	)
endif (OpenCV_FOUND)

ADD_CUSTOM_COMMAND(
    TARGET bundle
    POST_BUILD
    COMMAND mkdir ARGS ${CMAKE_BINARY_DIR}/src/glmixer.app/Contents/Resources/
    COMMAND cp -Rf ${CMAKE_SOURCE_DIR}/icons/glmixer.icns ${CMAKE_BINARY_DIR}/src/glmixer.app/Contents/Resources/
    COMMAND cp -Rf ${QT_LIBRARY_DIR}/QtGui.framework/Resources/qt_menu.nib ${CMAKE_BINARY_DIR}/src/glmixer.app/Contents/Resources/
)

else ( APPLE )

# create a normal application
add_executable(glmixer ${MYQT_SRCS} ${MYQT_UIS_H} ${MYQT_RCS_SRCS} )

# link the "qtproject" target against the Qt libraries. 
target_link_libraries(glmixer QtProperty ${QT_LIBRARIES} ${GLMIXER_LIBRARIES})

install(TARGETS glmixer DESTINATION /usr/local/bin)

endif( APPLE )




 
