ADD_EXECUTABLE(podofotxtextract TextExtractor.cpp podofotxtextract.cpp)

TARGET_LINK_LIBRARIES(podofotxtextract ${PODOFO_LIB})
SET_TARGET_PROPERTIES(podofotxtextract PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofotxtextract ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofotxtextract
	RUNTIME DESTINATION "bin")
