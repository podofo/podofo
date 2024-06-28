file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "*.h" "*.hpp")
list(FILTER SOURCE_FILES EXCLUDE REGEX "/\\._")
list(FILTER SOURCE_FILES EXCLUDE REGEX "^\\._")

# Set visual studio source group
source_group("" FILES ${SOURCE_FILES})

add_library(podofo_test STATIC ${SOURCE_FILES})
