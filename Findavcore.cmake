# Locate AVCORE library
# This module defines
# AVCORE_LIBRARY, the name of the library to link against
# AVCORE_FOUND, if false, do not try to link to AVCORE
# AVCORE_INCLUDE_DIR, where to find AVCORE.h
#

set( AVCORE_FOUND "NO" )

find_path( AVCORE_INCLUDE_DIR libavcore/avcore.h
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

#message( "AVCORE_INCLUDE_DIR is ${AVCORE_INCLUDE_DIR}" )

find_library( AVCORE_LIBRARY
  NAMES avcore
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

#message( "AVCORE_LIBRARY is ${AVCORE_LIBRARY}" )
if(AVCORE_LIBRARY)
set( AVCORE_FOUND "YES" )
endif(AVCORE_LIBRARY)

#message( "AVCORE_LIBRARY is ${AVCORE_LIBRARY}" )

MARK_AS_ADVANCED( AVCORE_FOUND )

