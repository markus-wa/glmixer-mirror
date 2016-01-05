
cmake_minimum_required (VERSION 2.6)
project (LIBESCAPIFFGL)

include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBESCAPIFFGL_SRCS
    escapi.cpp
    escapiFFGL.cpp
)

#find_library(PTHREAD_LIBRARY pthread REQUIRED)

add_library(escapiffgl SHARED ${LIBESCAPIFFGL_SRCS} )

target_link_libraries(escapiffgl ${OPENGL_LIBRARY} -Wl,--whole-archive FreeFrameGL -Wl,--no-whole-archive ${GLEW_LIBRARY})


