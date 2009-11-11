# Locate libvorbis libraries
# This module defines
# VORBIS_LIBRARIES, the name of the libraries to link against
# VORBIS_FOUND, if false, do not try to link
# VORBIS_INCLUDE_DIR, where to find header
#

set( VORBIS_FOUND "NO" )

find_path( VORBIS_INCLUDE_DIR vorbis/codec.h
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

find_library( VORBIS_LIBRARY_VORBIS
  NAMES vorbis
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

find_library( VORBIS_LIBRARY_VORBISENC
  NAMES vorbisenc 
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

set( VORBIS_LIBRARIES ${VORBIS_LIBRARY_VORBIS} ${VORBIS_LIBRARY_VORBISENC} )

if(VORBIS_LIBRARY_VORBIS)
set( VORBIS_FOUND "YES" )
endif(VORBIS_LIBRARY_VORBIS)
