# Locate SWSCALE FFMPEG library
# This module defines
# SWSCALE_LIBRARY, the name of the library to link against
# SWSCALE_FOUND, if false, do not try to link to AVCODEC
# SWSCALE_INCLUDE_DIR, where to find avcodec.h
#

set( SWSCALE_FOUND "NO" )

find_path( SWSCALE_INCLUDE_DIR libswscale/swscale.h
  HINTS
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include 
  /opt/include
  /mingw
)

find_library( SWSCALE_LIBRARY
  NAMES swscale
  HINTS
  PATH_SUFFIXES lib64 lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)

if(SWSCALE_LIBRARY)
set( SWSCALE_FOUND "YES" )
endif(SWSCALE_LIBRARY)

