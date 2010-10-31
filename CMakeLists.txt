project(glmixer) # the name of your project

cmake_minimum_required(VERSION 2.6.0)

FIND_PACKAGE(Subversion)
IF(Subversion_FOUND)
  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} GLMIXER)
  MESSAGE("Current revision is ${GLMIXER_WC_REVISION}")
ENDIF(Subversion_FOUND)

set(GLMIXER_VERSION "0.9")

# Expose CMAKE_INCLUDE_PATH and CMAKE_LIBARY_PATH to the GUI so users
# may set these values without needing to manipulate the environment.
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")
SET(USE_OPENCV ${USE_OPENCV} CACHE BOOL "CHECK if you want to compile with OpenCV library")

# avoid stupid cmake warning
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

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

# turn on verbose build to enable path autodiscovery in CDT
#SET(CMAKE_VERBOSE_MAKEFILE ON)

# defines used by preprocessor to integrate program version and SVN revision into the application
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLMIXER_VERSION=${GLMIXER_VERSION}" )
IF(GLMIXER_WC_REVISION)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLMIXER_REVISION=${GLMIXER_WC_REVISION}" )
ENDIF(GLMIXER_WC_REVISION)

# find the dependency libraries

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} )
set( CMAKE_FIND_LIBRARY_PREFIXES lib )
set( CMAKE_FIND_LIBRARY_SUFFIXES .a .lib .lib64)
set( CMAKE_LIBRARY_PATH 
  /opt/local/lib
  /opt/lib
  /usr/local/lib
  /usr/lib
  /mingw/lib 
  ${CMAKE_SYSTEM_LIBRARY_PATH}
)
set( CMAKE_INCLUDE_PATH 
  /opt/local/include
  /opt/include
  /usr/local/include
  /usr/include
  /mingw/include
  ${CMAKE_SYSTEM_INCLUDE_PATH}
)

find_package( Qt4 REQUIRED)

# HERE ARE THE OPENGL DEPENDENCIES

find_package( OpenGL REQUIRED)

set( GLMIXER_LIBRARIES  ${OPENGL_LIBRARY})
if (WIN32)
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} -lws2_32 )
endif (WIN32)
                    
find_library( GLEW_LIBRARY NAMES GLEW glew glew32)
if (GLEW_LIBRARY)
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${GLEW_LIBRARY} )
  find_path( GLEW_INCLUDE NAMES GL/glew.h )
  include_directories(${GLEW_INCLUDE})
else (GLEW_LIBRARY)
  message(FATAL_ERROR "No GLEW library found.")
endif (GLEW_LIBRARY)

# HERE ARE THE FFMPEG ESSENTIAL AND REQUIRED LIBRARIES

find_library( AVCODEC_LIBRARY NAMES avcodec)
if (AVCODEC_LIBRARY)
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${AVCODEC_LIBRARY} )
  find_path( FFMPEG_INCLUDE NAMES libavcodec/avcodec.h )
  include_directories(${FFMPEG_INCLUDE})
else (AVCODEC_LIBRARY)
  message(FATAL_ERROR "No avcodec library found.")
endif (AVCODEC_LIBRARY)

find_library( AVFORMAT_LIBRARY NAMES avformat)
if (AVFORMAT_LIBRARY)
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${AVFORMAT_LIBRARY} )
else (AVFORMAT_LIBRARY)
  message(FATAL_ERROR "No avformat library found.")
endif (AVFORMAT_LIBRARY)

find_library( AVUTIL_LIBRARY NAMES avutil)
if (AVUTIL_LIBRARY)
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${AVUTIL_LIBRARY} )
else (AVUTIL_LIBRARY)
  message(FATAL_ERROR "No avutil library found.")
endif (AVUTIL_LIBRARY)

find_library( SWSCALE_LIBRARY NAMES swscale)
if (SWSCALE_LIBRARY)
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${SWSCALE_LIBRARY} )
else (SWSCALE_LIBRARY)
  message(FATAL_ERROR "No swscale library found.")
endif (SWSCALE_LIBRARY)

find_library( AVCORE_LIBRARY NAMES avcore)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${AVCORE_LIBRARY} )

# BELLOW IS THE LIST OF FFMPEG OPTIONNAL DEPENDENCIES.
# ADD THE LIBRARIES AS REQUIRED BY YOUR INSTALLATION OF FFMPEG

find_library( THEORA_LIBRARY NAMES theora theoraenc theoradec)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${THEORA_LIBRARY} )

find_library( X264_LIBRARY NAMES x264)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${X264_LIBRARY} )

find_library( XVID_LIBRARY NAMES xvidcore)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${XVID_LIBRARY} )

find_library( FAAC_LIBRARY NAMES faac)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${FAAC_LIBRARY} )

find_library( MP3LAME_LIBRARY NAMES mp3lame)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${MP3LAME_LIBRARY} )

find_library( VORBIS_LIBRARY NAMES vorbis)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${VORBIS_LIBRARY} )

find_library( VORBISENC_LIBRARY NAMES vorbisenc)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${VORBISENC_LIBRARY} )

find_library( OGG_LIBRARY NAMES ogg)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${OGG_LIBRARY} )

find_library( ORC_LIBRARY NAMES orc-0.4)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${ORC_LIBRARY} )

find_library( SCHROEDINGER_LIBRARY NAMES schroedinger-1.0)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${SCHROEDINGER_LIBRARY} )

find_library( SPEEDX_LIBRARY NAMES speex)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${SPEEDX_LIBRARY} )

find_library( DIRACENC_LIBRARY NAMES dirac_encoder)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${DIRACENC_LIBRARY} )

find_library( VPX_LIBRARY NAMES vpx)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${VPX_LIBRARY} )

find_library( BZ2_LIBRARY NAMES bz2)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${BZ2_LIBRARY} )

find_library( Z_LIBRARY NAMES z)
set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${Z_LIBRARY} )

# HERE IS THE OPTIONNAL INSTALLATION OF OPENCV

if (USE_OPENCV)
    if( APPLE )
        find_library( OpenCV_LIBS NAMES OpenCV)
        if(OpenCV_LIBS)
            set( OpenCV_FOUND "YES" )
        endif(OpenCV_LIBS)
       include_directories(${OpenCV_LIBS}/Headers)
    else ( APPLE )
        find_package( OpenCV QUIET)
    endif( APPLE )
endif( USE_OPENCV )

# If there is OpenCV, then this application will use OpenCV
if (OpenCV_FOUND)
  message(STATUS "OpenCV Library found OK")
  set( GLMIXER_LIBRARIES  ${GLMIXER_LIBRARIES} ${OpenCV_LIBS}  )
endif (OpenCV_FOUND)

include(${CMAKE_SOURCE_DIR}/macros.cmake)

# tell cmake to process CMakeLists.txt in those subdirectories

add_subdirectory(libQtProperty)
add_subdirectory(src)
