project(glmixer) # the name of your project

set(GLMIXER_VERSION "0.5")

cmake_minimum_required(VERSION 2.6.0)

# Expose CMAKE_INCLUDE_PATH and CMAKE_LIBARY_PATH to the GUI so users
# may set these values without needing to manipulate the environment.
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} CACHE STRING "You may add additional search paths here. Use ; to separate multiple paths.")

# avoid stupid cmake warning
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

# force GCC to produce single-line errors for CDT
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0 -pipe -Wall")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0 -pipe -Wall -Wno-write-strings")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# turn on verbose build to enable path autodiscovery in CDT
SET(CMAKE_VERBOSE_MAKEFILE ON)


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLMIXER_VERSION=${GLMIXER_VERSION}" )

if( APPLE )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=10.4" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4" )
endif( APPLE )

# find the dependency libraries

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} )
find_package( Qt4 REQUIRED)
find_package( OpenGL REQUIRED)
find_package( avformat REQUIRED)
find_package( avcodec REQUIRED)
find_package( avutil REQUIRED )
find_package( swscale REQUIRED )
find_package( ZLIB REQUIRED)
find_package( BZip2 QUIET)
find_package( theora QUIET)
find_package( x264 QUIET)
find_package( xvid QUIET)
find_package( openjpeg QUIET)
find_package( vorbis QUIET)
find_package( mp3lame QUIET)
find_package( faac QUIET)
find_package( faad QUIET)

find_package( OpenCV QUIET)

if (AVCODEC_FOUND)
  message(STATUS "FFMPEG avcodec Library Found OK")
  include_directories(${AVCODEC_INCLUDE_DIR})
else (AVCODEC_FOUND)
  message(FATAL_ERROR "No avcodec library found.")  
endif (AVCODEC_FOUND)

if (AVFORMAT_FOUND)
  message(STATUS "FFMPEG avformat Library Found OK")
  include_directories(${AVFORMAT_INCLUDE_DIR})
else (AVFORMAT_FOUND)
  message(FATAL_ERROR "No FFMPEG avformat library found.")  
endif (AVFORMAT_FOUND)

if (AVUTIL_FOUND)
  message(STATUS "FFMPEG avutil Library Found OK")
  include_directories(${AVUTIL_INCLUDE_DIR})
else (AVUTIL_FOUND)
  message(FATAL_ERROR "No FFMPEG avutil library found.")  
endif (AVUTIL_FOUND)

if (SWSCALE_FOUND)
  message(STATUS "FFMPEG swscale Library Found OK")
  include_directories(${SWSCALE_INCLUDE_DIR})
else (SWSCALE_FOUND)
  message(FATAL_ERROR "No FFMPEG swscale library found.")  
endif (SWSCALE_FOUND)

if (OPENGL_FOUND)
  message(STATUS "OpenGL Library Found OK")
  include_directories(${OPENGL_INCLUDE_DIR})
else (OPENGL_FOUND)
  message(FATAL_ERROR "No OpenGL library found.")  
endif (OPENGL_FOUND)

if (WIN32)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} -lws2_32 )
endif (WIN32)

if (ZLIB_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${ZLIB_LIBRARY} )
  message(STATUS "ZLIB Library Found OK")
endif (ZLIB_FOUND)

if (BZIP2_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${BZIP2_LIBRARIES} )
  message(STATUS "BZIP2 Library Found OK")
endif (BZIP2_FOUND)

if (THEORA_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${THEORA_LIBRARY} )
  message(STATUS "THEORA Library Found OK")
endif (THEORA_FOUND)

if (X264_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${X264_LIBRARY} )
  message(STATUS "X264 Library Found OK")
endif (X264_FOUND)

if (OPENJPEG_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${OPENJPEG_LIBRARY} )
  message(STATUS "OPENJPEG Library Found OK")
endif (OPENJPEG_FOUND)

if (XVID_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${XVID_LIBRARY} )
  message(STATUS "XVID Library Found OK")
endif (XVID_FOUND)

if (VORBIS_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${VORBIS_LIBRARIES} )
  message(STATUS "VORBIS Library Found OK")
endif (VORBIS_FOUND)

if (MP3LAME_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${MP3LAME_LIBRARY} )
  message(STATUS "MP3LAME Library Found OK")
endif (MP3LAME_FOUND)

if (FAAC_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${FAAC_LIBRARY} )
  message(STATUS "FAAC Library Found OK")
endif (FAAC_FOUND)

if (FAAD_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${FAAD_LIBRARY} )
  message(STATUS "FAAD Library Found OK")
endif (FAAD_FOUND)

if (MATROSKA_FOUND)
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${MATROSKA_LIBRARY} )
  message(STATUS "MATROSKA Library Found OK")
endif (MATROSKA_FOUND)

# If there is OpenCV, then this application will use OpenCV
if (OpenCV_FOUND)
  message(STATUS "OpenCV Library found OK")
  set( EXTRA_LIBRARIES    ${EXTRA_LIBRARIES} ${OpenCV_LIBS} )
else (OpenCV_FOUND)
  message(STATUS "OpenCV Library Not found")
endif (OpenCV_FOUND)

set( GLMIXER_LIBRARIES   ${AVFORMAT_LIBRARY}
                        ${AVCODEC_LIBRARY}
                        ${AVUTIL_LIBRARY}
                        ${SWSCALE_LIBRARY}
                        ${EXTRA_LIBRARIES} )

# tell cmake to process CMakeLists.txt in that subdirectory
add_subdirectory(src)