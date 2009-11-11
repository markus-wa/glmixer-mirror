# Locate matroska library      
# This module defines
# MATROSKA_LIBRARY, the name of the library to link against
# MATROSKA_FOUND, if false, do not try to link
#

set( MATROSKA_FOUND "NO" )

find_library( MATROSKA_LIBRARY
  NAMES matroska
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

if(MATROSKA_LIBRARY)
set( MATROSKA_FOUND "YES" )
endif(MATROSKA_LIBRARY)
