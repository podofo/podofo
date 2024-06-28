file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "[^.]*")

# Set visual studio source group
source_group("" FILES ${SOURCE_FILES})

add_library(tools_private STATIC ${SOURCE_FILES})
target_link_libraries(tools_private podofo_private)
