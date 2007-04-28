ADD_EXECUTABLE(ObjectParserTest ObjectParserTest.cpp)
TARGET_LINK_LIBRARIES(ObjectParserTest ${PODOFO_LIB} ${PODOFO_LIB_DEPENDS})
SET_TARGET_PROPERTIES(ObjectParserTest PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(ObjectParserTest ${PODOFO_DEPEND_TARGET})

# Copy the test samples over to the build tree
ADD_CUSTOM_COMMAND(
    TARGET ObjectParserTest
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/objects" "${CMAKE_CURRENT_BINARY_DIR}/objects"
    )
