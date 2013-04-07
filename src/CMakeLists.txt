# the next line sets up include and link directories and defines some variables that we will use.

# this application uses QT with the following modules:
set(QT_USE_OPENGL    TRUE)
set(QT_USE_QTOPENGL  TRUE)
set(QT_USE_QTXML     TRUE)
set(QT_USE_QTSVG     TRUE)
set(QT_USE_QTMAIN    TRUE)

# add the corresponding path to include
include(${QT_USE_FILE})

#--------------------------------------------------------------------------------
# This sets the windows build that will need the special winmain@16 call. Qt provides
#  this for us in the qtmain.lib file. Using this cmake code will ensure we have it
#  linked into our build. Not needed on Unix/OS X/Linux which is why we have the 
#  IF(WIN32) conditional.
IF (WIN32)
    SET (QT_USE_QTMAIN TRUE)
ENDIF (WIN32)

# the variable "qtproject_SRCS" contains all .cpp files of this project
set(GLMIXER_SRCS
    common.cpp
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
    SharedMemoryManager.cpp
    SharedMemorySource.cpp
    SharedMemoryDialog.cpp
    SourceDisplayWidget.cpp
    View.cpp
    MixerView.cpp
    GeometryView.cpp
    LayersView.cpp
    RenderingView.cpp
    CatalogView.cpp
    ViewRenderWidget.cpp
    SelectionManager.cpp
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
    MixingToolboxWidget.cpp
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
    SharedMemoryDialog.ui
    MixingToolboxWidget.ui
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
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# this application uses the library QtProperty
include_directories(${LIBQTPROPERTY_SOURCE_DIR} )

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( ${GLMIXER_UIS_H} ${GLMIXER_SRCS})

#--------------------------------------------------------------------------------
# For Apple set the icns file containing icons (to do before declaring executable MACOSX_BUNDLE)
IF(APPLE)
  # set how it shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE glmixer.icns) 
  # set where in the bundle to put the icns file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/osx/glmixer.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # include the icns file in the target
  SET(GLMIXER_SRCS ${GLMIXER_SRCS} ${CMAKE_SOURCE_DIR}/osx/glmixer.icns)
ENDIF(APPLE)

# create the application
add_executable(glmixer MACOSX_BUNDLE WIN32 
	${GLMIXER_SRCS} 
	${GLMIXER_UIS_H} 
	${GLMIXER_RCS_SRCS} 
)	
# link the target against the libraries. 
target_link_libraries(glmixer QtProperty ${QT_LIBRARIES} ${GLMIXER_LIBRARIES})


IF(UNIX)

    IF(APPLE)
        # OSX cpack info
    	set (CPACK_SYSTEM_NAME "OSX_${CMAKE_OSX_ARCHITECTURES}")
    	set (CPACK_BUNDLE_NAME glmixer)
    	set (CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/osx/Info.plist)
    	set (CPACK_BUNDLE_ICON osx/${MACOSX_BUNDLE_ICON_FILE}) 
        set (CPACK_BINARY_DRAGNDROP ON)
    ELSE(APPLE)
        # Linux cpack info
    	set (CPACK_SYSTEM_NAME "${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
        set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Bruno Herbelin") 
        set (CPACK_DEBIAN_PACKAGE_NAME "GLMixer")
        set (CPACK_DEBIAN_PACKAGE_DESCRIPTION "Graphic Live Mixer: real-time video mixing software for live performance.")
        set (CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://code.google.com/p/glmixer/")
        set (CPACK_DEBIAN_PACKAGE_SECTION "Multimedia")
        set (CPACK_DEBIAN_PACKAGE_VERSION "${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}.${GLMIXER_WC_REVISION}")
        # autogenerate dependency information
        set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    ENDIF(APPLE)

ENDIF(UNIX)


# multiplatform cpack info
set (CPACK_PACKAGE_NAME "GLMixer")
set (CPACK_PACKAGE_VERSION_MAJOR "${GLMIXER_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${GLMIXER_VERSION_MINOR}")
set (CPACK_PACKAGE_VENDOR "Bruno Herbelin")
set (CPACK_PACKAGE_FILE_NAME "GLMixer_${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}_${GLMIXER_WC_REVISION}_${CPACK_SYSTEM_NAME}")
	
	
#--------------------------------------------------------------------------------
# Now the installation stuff below
#--------------------------------------------------------------------------------
SET(plugin_dest_dir bin)
SET(qtconf_dest_dir bin)
SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/glmixer")
IF(APPLE)
  SET(plugin_dest_dir GLMixer.app/Contents/MacOS)
  SET(qtconf_dest_dir GLMixer.app/Contents/Resources)
  SET(APPS "\${CMAKE_INSTALL_PREFIX}/GLMixer.app")
ENDIF(APPLE)
IF(WIN32)
  SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/glmixer.exe")
ENDIF(WIN32)

#--------------------------------------------------------------------------------
# Install the glmixer application, on Apple, the bundle is at the root of the
# install tree, and on other platforms it'll go into the bin directory.
INSTALL(TARGETS glmixer 
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
    )

IF(APPLE)

    #--------------------------------------------------------------------------------
    # Install needed Qt plugins by copying directories from the qt installation
    # One can cull what gets copied by using 'REGEX "..." EXCLUDE'
    INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${plugin_dest_dir}/plugins COMPONENT Runtime)
    
    #--------------------------------------------------------------------------------
    # install a qt.conf file
    # this inserts some cmake code into the install script to write the file
    INSTALL(CODE "
        file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt.conf\" \"\")
        " COMPONENT Runtime)
    
    
    #--------------------------------------------------------------------------------
    # install a qt_menu.nib files structure
    INSTALL(CODE "
        file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt_menu.nib\")
        file(COPY \"${QT_LIBRARY_DIR}/Resources/qt_menu.nib\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}\" NO_SOURCE_PERMISSIONS )
        " COMPONENT Runtime)    
        
    #--------------------------------------------------------------------------------
    # Use BundleUtilities to get all other dependencies for the application to work.
    # It takes a bundle or executable along with possible plugins and inspects it
    # for dependencies.  If they are not system dependencies, they are copied.
    
    # directories to look for dependencies
    SET(DIRS ${QT_LIBRARY_DIRS} ${CMAKE_LIBRARY_PATH})
    
    # Now the work of copying dependencies into the bundle/package
    # The quotes are escaped and variables to use at install time have their $ escaped
    # An alternative is the do a configure_file() on a script and use install(SCRIPT  ...).
    # Note that the image plugins depend on QtSvg and QtXml, and it got those copied
    # over.
    INSTALL(CODE "
        file(GLOB_RECURSE QTPLUGINS
          \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
        include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
        " COMPONENT Runtime)
        
ENDIF(APPLE)



# To Create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is created by including CPack
# And then there's ways to customize this as well
include(CPack)
	
	
	
	
	
	
 
