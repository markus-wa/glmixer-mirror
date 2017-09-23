
cmake_minimum_required (VERSION 2.6)
project (LIBBLACKMAGICFFGL)

## include_directories(${FREEFRAMEGL_DIR_PLUGINS})

set(LIBBLACKMAGICFFGL_SRCS
    DeckLinkAPIDispatch.cpp
    blackmagicPlayback.cpp
    blackmagicFFGL.cpp
)

find_library( PTHREAD_LIBRARY pthread)
find_library( CORE_LIBRARY CoreFoundation)

if(PTHREAD_LIBRARY)
    # declare lib
    add_library(blackmagicffgl MODULE ${LIBBLACKMAGICFFGL_SRCS} )
    # link plugin
    target_link_libraries(blackmagicffgl -Wl,-all_load FreeFrameGL ${OPENGL_LIBRARY} ${GLEW_LIBRARY} ${PTHREAD_LIBRARY} ${CORE_LIBRARY})
else()
    message(WARNING "blackmagicFFGL not compiled because of missing dependeny.")
endif()

