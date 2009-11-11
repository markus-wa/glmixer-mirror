# - Find zlib
# Find the native ZLIB includes and library
#
#  ZLIB_INCLUDE_DIR - where to find zlib.h, etc.
#  ZLIB_LIBRARIES   - List of libraries when using zlib.
#  ZLIB_FOUND       - True if zlib found.


IF (ZLIB_INCLUDE_DIR)
  # Already in cache, be silent
  SET(ZLIB_FIND_QUIETLY TRUE)
ENDIF (ZLIB_INCLUDE_DIR)

FIND_PATH(ZLIB_INCLUDE_DIR zlib.h
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

SET(ZLIB_NAMES z zlib zdll)
FIND_LIBRARY(ZLIB_LIBRARY NAMES ${ZLIB_NAMES} 
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

# handle the QUIETLY and REQUIRED arguments and set ZLIB_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZLIB DEFAULT_MSG ZLIB_LIBRARY ZLIB_INCLUDE_DIR)

IF(ZLIB_FOUND)
  SET( ZLIB_LIBRARIES ${ZLIB_LIBRARY} )
ELSE(ZLIB_FOUND)
  SET( ZLIB_LIBRARIES )
ENDIF(ZLIB_FOUND)

MARK_AS_ADVANCED( ZLIB_LIBRARY ZLIB_INCLUDE_DIR )
