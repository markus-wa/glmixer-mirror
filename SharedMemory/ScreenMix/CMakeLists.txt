
cmake_minimum_required (VERSION 2.6)
project (SCREENMIX)

# add the corresponding path to include
include(${QT_USE_FILE})

#include the shared memory manager dir
include_directories(${LIBSHAREDMEMORYMANAGER_SOURCE_DIR} )

#include the current source dir
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# do the qt-cmake magic
qt4_add_resources(SCREENMIX_RCS_SRCS icons.qrc)

# tell cmake to create .moc files for all files that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc( screencapture.cpp )

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(screenMix 
               screencapture.cpp 
               main.cpp               
               ${SCREENMIX_RCS_SRCS}
)

target_link_libraries(screenMix SharedMemoryManager ${QT_LIBRARIES} )

