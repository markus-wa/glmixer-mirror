##
## CMake configuration
## 
project(glmixer)
cmake_minimum_required(VERSION 2.6.0)
include(${CMAKE_SOURCE_DIR}/macros.cmake)

# Use SVN to read repository version
FIND_PACKAGE(Subversion)
IF(Subversion_FOUND)
  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} GLMIXER)
  MESSAGE("Current revision is ${GLMIXER_WC_REVISION}")
ENDIF(Subversion_FOUND)

set(GLMIXER_VERSION_MAJOR 1)
set(GLMIXER_VERSION_MINOR 1)

# Expose CMAKE_INCLUDE_PATH and CMAKE_LIBARY_PATH to the GUI so users
# may set these values without needing to manipulate the environment.
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")

# Propose two build configurations
#set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Compilation configuration" FORCE)
if(DEFINED CMAKE_BUILD_TYPE)
    set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release" )
endif()

if( APPLE )

#set(CMAKE_OSX_ARCHS "i386;x86_64" CACHE STRING "Compilation architecture" FORCE)
if(DEFINED CMAKE_OSX_ARCHITECTURES)
    set_property( CACHE CMAKE_OSX_ARCHITECTURES PROPERTY STRINGS "i386;x86_64" )
endif()

#set(CMAKE_OSX_TARGETS "10.5;10.6;10.7" CACHE STRING "Target OSX version (Min requirement)" FORCE)
if(DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
	set_property( CACHE CMAKE_OSX_DEPLOYMENT_TARGET PROPERTY STRINGS "10.5;10.6;10.7" )
endif()

endif( APPLE )

# avoid stupid cmake warning
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

##
## COMPILATION SETTINGS
##

# force GCC to produce single-line errors for CDT
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0 -pipe -Wall")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS -fmessage-length=0 -pipe -Wall -Wno-write-strings")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# force Linker to link with / without console under Windows
IF(WIN32)
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -mingw32 -Wl,-subsystem,console")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -mingw32 -Wl,-subsystem,windows")
ENDIF(WIN32)

# defines used by preprocessor to integrate program version and SVN revision into the application
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLMIXER_VERSION=${GLMIXER_VERSION_MAJOR}.${GLMIXER_VERSION_MINOR}" )
IF(GLMIXER_WC_REVISION)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLMIXER_REVISION=${GLMIXER_WC_REVISION}" )
ENDIF(GLMIXER_WC_REVISION)

##
## LIBRARIES 
##

# Propose two library link modes> static or shared
set(GLMIXER_LINK_STATIC OFF CACHE BOOL "Preferably link with static libraries (when available)")

set( CMAKE_FIND_LIBRARY_SUFFIXES )
if( GLMIXER_LINK_STATIC )
    message(STATUS "Searching for static libraries preferably." )
    # set the possible suffix for library files in favor of static libs
    if( APPLE )
    	set( CMAKE_FIND_LIBRARY_SUFFIXES .a .so .so.0 .dylib .framework)
    else( APPLE)
    	if (WIN32)
      		set( CMAKE_FIND_LIBRARY_SUFFIXES .a .lib .lib64 .dll )
      	else (WIN32)
      		# not APPLE, not WINDOWS ; must be unix !
    		set( CMAKE_FIND_LIBRARY_SUFFIXES  .a .so .so.0 .so.1 )
    	endif (WIN32)
    endif( APPLE )
else( GLMIXER_LINK_STATIC )
    message(STATUS "Searching for shared libraries preferably" )
    # set the possible suffix for library files in favor of dynamic libs 
    if( APPLE )
    	set( CMAKE_FIND_LIBRARY_SUFFIXES .so .so.0 .dylib .framework .a )
    else( APPLE)
    	if (WIN32)
      		set( CMAKE_FIND_LIBRARY_SUFFIXES .dll .lib .lib64 .a )
      	else (WIN32)
      		# not APPLE, not WINDOWS ; must be unix !
    		set( CMAKE_FIND_LIBRARY_SUFFIXES  .so .so.0 .so.1 .a )
    	endif (WIN32)
    endif( APPLE )
endif( GLMIXER_LINK_STATIC )

set( CMAKE_LIBRARY_PATH 
  /opt/local/lib
  /opt/lib
  /usr/local/lib
  /usr/lib
  /mingw/bin 
  /mingw/lib 
  ~/Library/Frameworks
  /System/Library/Frameworks
  ${CMAKE_SYSTEM_LIBRARY_PATH}
)


# HERE ARE THE BASIC DEPENDENCIES
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} )
find_package( Qt4 REQUIRED)
find_package( OpenGL REQUIRED)
set( GLMIXER_LIBRARIES  ${OPENGL_LIBRARY})

# SOME HINTS FOR THE FIND_LIBRARY BELLOW
set( CMAKE_INCLUDE_PATH 
  /opt/local/include
  /opt/include
  /usr/local/include
  /usr/include
  /mingw/include
  ${CMAKE_SYSTEM_INCLUDE_PATH}
)

# HERE IS THE OPTIONNAL INSTALLATION OF OPENCV
SET(USE_OPENCV ${USE_OPENCV} CACHE BOOL "CHECK if you want to compile with OpenCV library")

if (USE_OPENCV)

unset(OPENCV_CORE_LIBRARY CACHE)
unset(OPENCV_IMGPROC_LIBRARY CACHE)
unset(OPENCV_HIGHGUI_LIBRARY CACHE)

find_library( OPENCV_CORE_LIBRARY NAMES opencv_core opencv_core230)
find_library( OPENCV_IMGPROC_LIBRARY NAMES opencv_imgproc opencv_imgproc230)
find_library( OPENCV_HIGHGUI_LIBRARY NAMES opencv_highgui opencv_highgui230)
# if we find the opencv libs required, then use opencv
if (OPENCV_CORE_LIBRARY AND OPENCV_IMGPROC_LIBRARY AND OPENCV_HIGHGUI_LIBRARY)
	set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${OPENCV_IMGPROC_LIBRARY} ${OPENCV_HIGHGUI_LIBRARY} ${OPENCV_CORE_LIBRARY})
	message(STATUS "Found OpenCV libraries (${OPENCV_IMGPROC_LIBRARY} ${OPENCV_HIGHGUI_LIBRARY} ${OPENCV_CORE_LIBRARY}).")
	
	if( APPLE )
	    SET(USE_QUICKTIME ${USE_QUICKTIME} CACHE BOOL "CHECK if you want to compile with QuickTime library")
	
		if(USE_QUICKTIME)
			find_package( QuickTime REQUIRED)
			set( GLMIXER_LIBRARIES ${GLMIXER_LIBRARIES} ${QUICKTIME_LIBRARY} )
			set(OSX_ARCHITECTURES "i386")
			message(STATUS "Set the variable CMAKE_OSX_ARCHITECTURE to i386 because QuickTime is i386 only.")
		else(USE_QUICKTIME)
			find_library( QTKIT_LIBRARY NAMES QTKit )
			if(QTKIT_LIBRARY)
				set( GLMIXER_LIBRARIES ${GLMIXER_LIBRARIES} ${QTKIT_LIBRARY} )
				message(STATUS "Found QTKit framework.")
			endif(QTKIT_LIBRARY)
		endif(USE_QUICKTIME)
		find_library( COCOA_LIBRARY NAMES Cocoa )
		find_library( QUARTZCORE_LIBRARY NAMES QuartzCore )
		if (COCOA_LIBRARY AND QUARTZCORE_LIBRARY)
			set( GLMIXER_LIBRARIES ${GLMIXER_LIBRARIES} ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY}  )
			message(STATUS "OpenCV OSX dependencies OK.")
			set(USE_OPENCV true)
		endif (COCOA_LIBRARY AND QUARTZCORE_LIBRARY)
	else ( APPLE )
		if( UNIX )
			find_library( V4L_LIBRARY NAMES v4l1 )
			if (V4L_LIBRARY)
				set( GLMIXER_LIBRARIES ${GLMIXER_LIBRARIES} ${V4L_LIBRARY}  )
				message(STATUS "OpenCV UNIX dependencies OK.")
				set(USE_OPENCV true)
			endif (V4L_LIBRARY)
		else( UNIX )
			set(USE_OPENCV true)
		endif( UNIX )
	
	endif ( APPLE )
	
endif (OPENCV_CORE_LIBRARY AND OPENCV_IMGPROC_LIBRARY AND OPENCV_HIGHGUI_LIBRARY)

endif (USE_OPENCV)

if (USE_OPENCV)
	message(STATUS "Using OpenCV.")
endif (USE_OPENCV)


# HERE ARE THE FFMPEG ESSENTIAL AND REQUIRED LIBRARIES

SET(GLMIXER_DEPENDENCIES avformat avcodec avutil swscale)
    
FOREACH(GLMIXER_DEPENDENCY ${GLMIXER_DEPENDENCIES})
    STRING(TOUPPER ${GLMIXER_DEPENDENCY} _upper_glmixer_dependency)
    unset(${_upper_glmixer_dependency}_LIBRARY CACHE)
    find_library(${_upper_glmixer_dependency}_LIBRARY ${GLMIXER_DEPENDENCY})
	if (${_upper_glmixer_dependency}_LIBRARY)
		set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${${_upper_glmixer_dependency}_LIBRARY} )
		message( STATUS "Found ${GLMIXER_DEPENDENCY}: ${${_upper_glmixer_dependency}_LIBRARY}")
	else (${_upper_glmixer_dependency}_LIBRARY)
	    message( FATAL_ERROR "${GLMIXER_DEPENDENCY} library NOT found but required.")
	endif (${_upper_glmixer_dependency}_LIBRARY)
ENDFOREACH(GLMIXER_DEPENDENCY)


# BELLOW IS THE LIST OF FFMPEG OPTIONNAL DEPENDENCIES.
# ADD THE LIBRARIES AS REQUIRED BY YOUR INSTALLATION OF FFMPEG

SET(GLMIXER_UTILITIES avcore theora modplug theoraenc theoradec x264 xvidcore faac gsm
                      mp3lame vorbis vorbisenc ogg orc-0.4 schroedinger-1.0 va png14
                      speex dirac_encoder jpeg openjpeg vpx bz2 z)
    
FOREACH(GLMIXER_UTILITY ${GLMIXER_UTILITIES})
    STRING(TOUPPER ${GLMIXER_UTILITY} _upper_glmixer_utility)
    # unset the variable to force the search for the library
    unset(${_upper_glmixer_utility}_LIBRARY CACHE)
    find_library(${_upper_glmixer_utility}_LIBRARY ${GLMIXER_UTILITY})
	if (${_upper_glmixer_utility}_LIBRARY)
		set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${${_upper_glmixer_utility}_LIBRARY} )
		message( STATUS "Found ${GLMIXER_UTILITY}: ${${_upper_glmixer_utility}_LIBRARY}")
	endif (${_upper_glmixer_utility}_LIBRARY)
ENDFOREACH(GLMIXER_UTILITY)

# HERE IS THE INSTALLATION OF WINDOWS SOCK LIB 

if (WIN32)
	# HERE IS THE INSTALLATION WINDOWS SOCK LIB 
 	set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} -lws2_32 )
endif (WIN32)

# HERE IS THE GLEW DEPENDENCY

unset(GLEW_LIBRARY CACHE)
find_library( GLEW_LIBRARY NAMES GLEW glew glew32)
if (GLEW_LIBRARY)
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${GLEW_LIBRARY} )
  find_path( GLEW_INCLUDE NAMES GL/glew.h )
  include_directories(${GLEW_INCLUDE})
  message( STATUS "Found GLEW: ${GLEW_LIBRARY}.")
else (GLEW_LIBRARY)
  message(FATAL_ERROR "GLEW library NOT found but required.")
endif (GLEW_LIBRARY)

##
## tell cmake to process CMakeLists.txt in those subdirectories
##

add_subdirectory(libQtProperty)
add_subdirectory(src)
