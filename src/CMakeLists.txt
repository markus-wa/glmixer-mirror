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
if (USE_OPENCV)
  add_definitions(-DOPEN_CV )
  set( MYQT_SRCS  ${MYQT_SRCS} OpencvSource.cpp )
endif (USE_OPENCV)

# do the qt-cmake magic
qt4_wrap_ui(MYQT_UIS_H ${MYQT_UIS})
qt4_add_resources(MYQT_RCS_SRCS ${MYQT_RCS})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# this application uses the library QtProperty
include_directories(${LIBQTPROPERTY_SOURCE_DIR} )

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( ${MYQT_UIS_H} ${MYQT_SRCS})

# create the application
add_executable(glmixer ${MYQT_SRCS} ${MYQT_UIS_H} ${MYQT_RCS_SRCS} )	

# link the target against the libraries. 
target_link_libraries(glmixer QtProperty ${QT_LIBRARIES} ${GLMIXER_LIBRARIES})


if( APPLE )
		
		
	install(TARGETS glmixer DESTINATION bin)
	install(FILES runglmixer DESTINATION bin
	
	set (CPACK_BUNDLE_NAME GLMixer)
	set (CPACK_BUNDLE_ICON )
	set (CPACK_BUNDLE_PLIST )
	set (CPACK_BUNDLE_STARTUP_COMMAND )
	
	#set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
	set (CPACK_PACKAGE_VERSION_MAJOR "${GLMIXER_VERSION_MAJOR}")
	set (CPACK_PACKAGE_VERSION_MINOR "${GLMIXER_VERSION_MINOR}")
	
	include (CPack)
		


else ( APPLE )
	
	install(TARGETS glmixer DESTINATION /usr/local/bin)

endif( APPLE )




 
