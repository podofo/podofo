file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "[^.]*")

# Set visual studio source group
source_group("" FILES ${SOURCE_FILES})

if(NOT PODOFO_HAVE_JPEG_LIB)
    # Remove Jpeg source if not needed
    list(REMOVE_ITEM SOURCE_FILES
        JpegCommon.h
        JpegCommon.cpp
    )
endif()

if (NOT MSVC)
    # Fixes "relocation against hidden symbol can not
    # be used when making a shared object"
    add_compile_options(-fPIC)
endif()

add_library(podofo_private STATIC ${SOURCE_FILES})

target_link_libraries(podofo_private PRIVATE ${PODOFO_LIB_DEPENDS})
