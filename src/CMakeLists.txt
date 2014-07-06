# the next line SETs up include and link directories and defines some variables that we will use.

# this application uses QT with the following modules:
SET(QT_USE_QTOPENGL  TRUE)
SET(QT_USE_QTXML     TRUE)
SET(QT_USE_QTSVG     TRUE)
SET(QT_USE_QTNETWORK    TRUE)
SET(QT_USE_QTWEBKIT    TRUE)


if( WIN32 )
    #--------------------------------------------------------------------------------
    # This SETs the windows build that will need the special winmain@16 call. Qt provides
    #  this for us in the qtmain.lib file. Using this cmake code will ensure we have it
    #  linked into our build. Not needed on Unix/OS X/Linux which is why we have the
    #  IF(WIN32) conditional.
    set(QT_USE_QTMAIN TRUE)

    # force Linker to link with / without console under Windows
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}  -mingw32 -Wl,-subsystem,console,--enable-auto-import")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -mingw32 -Wl,-subsystem,windows,--enable-auto-import")
endif( WIN32 )

# add the corresponding path to include
include(${QT_USE_FILE})

# the variable "GLMIXER_SRCS" contains all .cpp files of this project
SET(GLMIXER_SRCS
    common.cpp
    glRenderWidget.cpp
    VideoFileDialogPreview.cpp
    VideoFileDialog.cpp
    VideoFileDisplayWidget.cpp
    VideoFile.cpp
    Source.cpp
    VideoSource.cpp
    CaptureSource.cpp
    SvgSource.cpp
    WebSource.cpp
    AlgorithmSource.cpp
    AlgorithmSelectionDialog.cpp
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
    PropertyBrowser.cpp
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
    LayoutToolboxWidget.cpp
    OpenSoundControlManager.cpp
    NewSourceDialog.cpp
    WebSourceCreationDialog.cpp
)

SET(GLMIXER_RCS
    ../icons.qrc
    ../textures.qrc
    ../images.qrc
)

SET(GLMIXER_UIS
    glmixer.ui
    VideoFileDialog.ui
    AlgorithmSelectionDialog.ui
    UserPreferencesDialog.ui
    GammaLevelsDialog.ui
    MixingToolboxWidget.ui
    LayoutToolboxWidget.ui
    NewSourceDialog.ui
    WebSourceCreationDialog.ui
)

# If SharedMemory was set, then link with its source files
if(USE_SHAREDMEMORY)

    add_definitions(-DSHM)
    SET(GLMIXER_SRCS  ${GLMIXER_SRCS}
        SharedMemorySource.cpp
        SharedMemoryDialog.cpp
    )

    SET(GLMIXER_UIS  ${GLMIXER_UIS}
        SharedMemoryDialog.ui
    )

    include_directories(${LIBSHAREDMEMORYMANAGER_SOURCE_DIR} )
    SET(GLMIXER_LIBRARIES SharedMemoryManager ${GLMIXER_LIBRARIES} )

elseif(USE_SHAREDMEMORY)
    remove_definitions(-DSHM)
endif(USE_SHAREDMEMORY)

if( USE_SPOUT )
    add_definitions(-DSPOUT)
    message( STATUS "Using SPOUT Library")
elseif( USE_SPOUT )
    remove_definitions(-DSPOUT)
endif( USE_SPOUT )

# If FreeframeGL was set, then link with its source files
if(USE_FREEFRAMEGL)
    add_definitions(-DFFGL)

    SET( GLMIXER_SRCS  ${GLMIXER_SRCS}
        FFGLPluginInstances.cpp
        FFGLPluginSourceStack.cpp
        FFGLPluginSource.cpp
        FFGLPluginBrowser.cpp
        FFGLSource.cpp
        FFGLSourceCreationDialog.cpp
        FFGLPluginSourceShadertoy.cpp
        GLSLCodeEditor.cpp
        GLSLCodeEditorWidget.cpp
        GLSLSyntaxHighlighter.cpp
    )

    SET_SOURCE_FILES_PROPERTIES(FFGLPluginInstances.cpp PROPERTIES COMPILE_FLAGS -fno-strict-aliasing )

    SET(GLMIXER_UIS  ${GLMIXER_UIS}
        GLSLCodeEditorWidget.ui
        FFGLSourceCreationDialog.ui
    )

    SET(GLMIXER_RCS  ${GLMIXER_RCS}
        ${CMAKE_CURRENT_BINARY_DIR}/../FreeFramePlugins/plugins.qrc
    )

    SET(GLMIXER_LIBRARIES FreeFrameGL ${GLMIXER_LIBRARIES} )


elseif(USE_FREEFRAMEGL)
    remove_definitions(-DFFGL)
endif(USE_FREEFRAMEGL)

# If there is OpenCV, then this application will use OpenCV
if(USE_OPENCV)
    add_definitions(-DOPEN_CV )
    SET( GLMIXER_SRCS  ${GLMIXER_SRCS}
        OpencvSource.cpp
        CameraDialog.cpp
    )

    SET(GLMIXER_UIS  ${GLMIXER_UIS}
        CameraDialog.ui
    )

elseif(USE_OPENCV)
    remove_definitions(-DOPEN_CV)
endif(USE_OPENCV)

#include the current source dir
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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

# translations
SET(LANG
    "${CMAKE_SOURCE_DIR}/locals/trans_fr.ts"
    "${CMAKE_SOURCE_DIR}/locals/trans_cz.ts"
)
#qt4_create_translation(GLMIXER_QM  ${GLMIXER_UIS_H} ${GLMIXER_SRCS} ${LANG})
qt4_add_translation(GLMIXER_QM ${LANG})

# multiplatform cpack info
SET(CPACK_PACKAGE_NAME "GLMixer")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graphic Live Mixer")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "${GLMIXER_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${GLMIXER_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${GLMIXER_WC_REVISION}")
SET(CPACK_PACKAGE_VENDOR "bhbn")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}_${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}")
SET(CPACK_SOURCE_IGNORE_FILES "/\\\\.svn/;" )

#--------------------------------------------------------------------------------
# For Apple set the icns file containing icons (to do before declaring executable MACOSX_BUNDLE)
IF(APPLE)
    # set how it shows up in the Info.plist file
    SET(MACOSX_BUNDLE_ICON_FILE glmixer.icns)
    # set where in the bundle to put the icns file
    SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/osx/glmixer.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # include the icns file in the target
    SET(GLMIXER_SRCS ${GLMIXER_SRCS} ${CMAKE_SOURCE_DIR}/osx/glmixer.icns)

    # create the application
    add_executable(glmixer MACOSX_BUNDLE
            ${GLMIXER_SRCS}
            ${GLMIXER_UIS_H}
            ${GLMIXER_RCS_SRCS}
            ${GLMIXER_QM}
    )

    # OSX cpack info
    set( CPACK_SYSTEM_NAME "OSX_${CMAKE_OSX_ARCHITECTURES}")
    set( CPACK_BUNDLE_NAME glmixer)
    set( CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/osx/Info.plist)
    set( CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/osx/${MACOSX_BUNDLE_ICON_FILE})
    set( CPACK_BINARY_DRAGNDROP ON)

ELSE(APPLE)

    IF(WIN32)

        # This sets up the exe icon for windows under mingw.
        set(RES_FILES "")
        if(MINGW)
            set(RES_FILES "${CMAKE_SOURCE_DIR}/win/glmixer.rc")
            set(CMAKE_RC_COMPILER_INIT windres)
            ENABLE_LANGUAGE(RC)
            SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
        endif(MINGW)

        # create the application
        add_executable(glmixer WIN32
                ${GLMIXER_SRCS}
                ${GLMIXER_UIS_H}
                ${GLMIXER_RCS_SRCS}
                ${GLMIXER_QM}
                ${RES_FILES}
        )

        SET(CPACK_SYSTEM_NAME "windows")
        SET(CPACK_GENERATOR "NSIS")
        SET(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/win\\\\glmixer.ico")
        SET(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/win\\\\glmixer.ico")
        SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/win\\\\glmixer.png")
        SET(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_SOURCE_DIR}/win\\\\glmixer.ico")
        SET(CPACK_NSIS_DISPLAY_NAME "GLMixer ${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}")
        SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\code.google.com\\\\p\\\\glmixer")
        SET(CPACK_NSIS_CONTACT "bruno.herbelin@gmail.com")
        SET(CPACK_NSIS_MODIFY_PATH OFF)
        SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL  ON)
        SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "glmixer.exe")
        SET(CPACK_NSIS_MENU_LINKS "https://code.google.com/p/glmixer" "GLMixer Web Site" "plugins" "GLMixer Plugins")

    ELSE(WIN32)

        # create the application
        add_executable(glmixer
                ${GLMIXER_SRCS}
                ${GLMIXER_UIS_H}
                ${GLMIXER_RCS_SRCS}
                ${GLMIXER_QM}
        )

        # Linux cpack info
        SET (CPACK_SYSTEM_NAME "${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
        SET (CPACK_DEBIAN_PACKAGE_MAINTAINER "Bruno Herbelin")
        SET (CPACK_DEBIAN_PACKAGE_NAME "GLMixer")
        SET (CPACK_DEBIAN_PACKAGE_DESCRIPTION "Graphic Live Mixer: real-time video mixing software for live performance.")
        SET (CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://code.google.com/p/glmixer/")
        SET (CPACK_DEBIAN_PACKAGE_SECTION "Multimedia")
        SET (CPACK_DEBIAN_PACKAGE_VERSION "${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}.${GLMIXER_WC_REVISION}")
        # autogenerate dependency information
        SET (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

    ENDIF(WIN32)

ENDIF(APPLE)

# link the target against the libraries.
target_link_libraries(glmixer QtProperty OSCPack ${QT_LIBRARIES} ${GLMIXER_LIBRARIES})



#--------------------------------------------------------------------------------
# Now the installation stuff below
#--------------------------------------------------------------------------------
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}_${GLMIXER_WC_REVISION}_${CPACK_SYSTEM_NAME}")
SET(plugin_dest_dir bin)
SET(qtconf_dest_dir bin)

IF(APPLE)
      SET(plugin_dest_dir GLMixer.app/Contents/MacOS)
      SET(qtconf_dest_dir GLMixer.app/Contents/Resources)
      SET(APPS "\${CMAKE_INSTALL_PREFIX}/GLMixer.app")
ELSE(APPLE)
    IF(WIN32)
        SET(APPS "\${CMAKE_INSTALL_PREFIX}/glmixer.exe")
        SET(CPACK_PACKAGE_EXECUTABLES "glmixer" "GLMixer")
    ELSE(WIN32)
        SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/glmixer")
    ENDIF(WIN32)
ENDIF(APPLE)

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

ELSE(APPLE)

  IF(WIN32)

    # uninstall step to remove registry entry
    SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "DeleteRegKey HKCU 'Software\\\\bhbn'")

    # apparently useless, but harmless too..
    INCLUDE(InstallRequiredSystemLibraries)

    # use cmake tools for getting DLL dependencies : I use a custom GetPrerequisites.cmake to support MINGW objdump
    INCLUDE(GetPrerequisites)

    # read full name of executable
    GET_TARGET_PROPERTY(GLMIXER_BINARY_LOCATION glmixer LOCATION)

    # call the function get_prerequisites to fill the list DEPENDENCIES
    message( STATUS "Looking for DLL dependencies:" )
    GET_PREREQUISITES(@GLMIXER_BINARY_LOCATION@ DEPENDENCIES 1 0 "" "${SPOUT_INCLUDE}")

    # request the installation of each dll file of the DEPENDENCIES list
    FOREACH(DEPENDENCY ${DEPENDENCIES})
        message(STATUS "  - ${DEPENDENCY}")
        INSTALL(FILES ${DEPENDENCY} DESTINATION bin COMPONENT Runtime)
    ENDFOREACH(DEPENDENCY)

    IF(MINGW)
        INSTALL(FILES "/MinGW/bin/mingwm10.dll" DESTINATION bin COMPONENT Runtime)
    ENDIF(MINGW)

  ENDIF(WIN32)

ENDIF(APPLE)



# To Create a package, one can run "cpack -G DragNDrop CPackConfig.cmake" on Mac OS X
# where CPackConfig.cmake is created by including CPack
# And then there's ways to customize this as well
include(CPack)







