# the next line sets up include and link directories and defines some variables that we will use.

# this application uses QT with the following modules:
set(QT_USE_OPENGL    TRUE)
set(QT_USE_QTOPENGL  TRUE)
set(QT_USE_QTXML     TRUE)
set(QT_USE_QTSVG     TRUE)
set(QT_USE_QTMAIN    TRUE)
# add the corresponding path to include
include(${QT_USE_FILE})

# the variable "qtproject_SRCS" contains all .cpp files of this project
set(GLMIXER_SRCS
    MixSource.cpp
    glRenderWidget.cpp
    CameraDialog.cpp
    VideoFileDialogPreview.cpp
    VideoFileDialog.cpp
    VideoFileDisplayWidget.cpp
    VideoFile.cpp
    Source.cpp
    VideoSource.cpp
    CaptureSource.cpp
    SvgSource.cpp
    AlgorithmSource.cpp
    AlgorithmSelectionDialog.cpp
    SourceDisplayWidget.cpp
    View.cpp
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
    AxisCursor.cpp
    LineCursor.cpp
    FuzzyCursor.cpp
    UserPreferencesDialog.cpp
    GammaLevelsWidget.cpp
    SessionSwitcher.cpp
    SessionSwitcherWidget.cpp
    glmixer.cpp
    main.cpp
    video_rec.c
    testbuttonframe.cpp
    testwheelframe.cpp
)

set(GLMIXER_RCS
    ../icons.qrc
    ../textures.qrc
    ../images.qrc
)

set(GLMIXER_UIS
    glmixer.ui
    VideoFileDialog.ui
    CameraDialog.ui
    AlgorithmSelectionDialog.ui
    UserPreferencesDialog.ui
    GammaLevelsDialog.ui
)


# If there is OpenCV, then this application will use OpenCV
if (USE_OPENCV)
  add_definitions(-DOPEN_CV )
  set( GLMIXER_SRCS  ${GLMIXER_SRCS} OpencvSource.cpp )
endif (USE_OPENCV)

# do the qt-cmake magic
qt4_wrap_ui(GLMIXER_UIS_H ${GLMIXER_UIS})
qt4_add_resources(GLMIXER_RCS_SRCS ${GLMIXER_RCS})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# this application uses the library QtProperty
include_directories(${LIBQTPROPERTY_SOURCE_DIR} )

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( ${GLMIXER_UIS_H} ${GLMIXER_SRCS})

# create the application
add_executable(glmixer ${GLMIXER_SRCS} ${GLMIXER_UIS_H} ${GLMIXER_RCS_SRCS} )	

# link the target against the libraries. 
target_link_libraries(glmixer QtProperty ${QT_LIBRARIES} ${GLMIXER_LIBRARIES})

# what to install and where?
install(TARGETS glmixer DESTINATION bin)

# multiplatform cpack info
set (CPACK_PACKAGE_VERSION_MAJOR "${GLMIXER_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${GLMIXER_VERSION_MINOR}")
set (CPACK_PACKAGE_VENDOR "Bruno Herbelin")
	
	
if( APPLE )

# TODO : 
#  foreach (it ${CMAKE_OSX_ARCHITECTURES})
#   set(ditto_ARGS ${ditto_ARGS} --arch ${it})
#  endforeach(it)


	GLMIXER_INSTALL_FRAMEWORKS( NAMES QtCore QtGui QtOpenGL QtSvg QtXml
	  FROM ${QT_LIBRARY_DIR} TO lib 
	)
	
	GLMIXER_INSTALL_DYLIBS( NAMES libqjpeg.dylib libqico.dylib
	  FROM ${QT_PLUGINS_DIR}/imageformats TO lib 
	)
	
	install(DIRECTORY ${QT_LIBRARY_DIR}/QtGui.framework/Resources/qt_menu.nib DESTINATION lib/Resources)
	
	set (CPACK_GENERATOR Bundle)
	SET(CPACK_PACKAGE_FILE_NAME "GLMixer_${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}_r${GLMIXER_WC_REVISION}_OSX_${CMAKE_OSX_ARCHITECTURES}")
	SET(CPACK_SYSTEM_NAME "OSX")
	 	
	set (CPACK_BUNDLE_NAME GLMixer)
	set (CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/osx/glmixer.icns)
	set (CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/osx/Info.plist)
	set (CPACK_BUNDLE_STARTUP_COMMAND ${CMAKE_SOURCE_DIR}/osx/runglmixer)
	
	#set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
	
	include (CPack)
		
endif( APPLE )




 
