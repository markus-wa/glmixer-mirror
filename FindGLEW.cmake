#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND
# GLEW_INCLUDE_PATH
# GLEW_LIBRARY
# 


FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
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
  DOC "The directory where GL/glew.h resides"
)
 
FIND_LIBRARY( GLEW_LIBRARY
	NAMES GLEW glew glew32
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
	DOC "The GLEW library"
)

IF (GLEW_INCLUDE_PATH)
	SET( GLEW_FOUND "YES" CACHE STRING "Set to YES if GLEW is found, not set otherwise")
ENDIF (GLEW_INCLUDE_PATH)

MARK_AS_ADVANCED( GLEW_FOUND )