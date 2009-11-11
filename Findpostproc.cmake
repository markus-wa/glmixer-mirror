# Locate POSTPROC FFMPEG library
# This module defines
# POSTPROC_LIBRARY, the name of the library to link against
# POSTPROC_FOUND, if false, do not try to link to AVCODEC
# POSTPROC_INCLUDE_DIR, where to find avcodec.h
#

set( POSTPROC_FOUND "NO" )

find_path( POSTPROC_INCLUDE_DIR libpostproc/postprocess.h
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

find_library( POSTPROC_LIBRARY
  NAMES postproc
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

if(POSTPROC_LIBRARY)
set( POSTPROC_FOUND "YES" )
endif(POSTPROC_LIBRARY)

