# Locate openjpeg library  
# This module defines
# OPENJPEG_LIBRARY, the name of the library to link against
# OPENJPEG_FOUND, if false, do not try to link
# OPENJPEG_INCLUDE_DIR, where to find header
#

set( OPENJPEG_FOUND "NO" )

find_path( OPENJPEG_INCLUDE_DIR openjpeg.h
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

find_library( OPENJPEG_LIBRARY
  NAMES openjpeg
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

if(OPENJPEG_LIBRARY)
  set( OPENJPEG_FOUND "YES" )
endif(OPENJPEG_LIBRARY)
