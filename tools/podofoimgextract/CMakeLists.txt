ADD_EXECUTABLE(podofoimgextract ImageExtractor.cpp podofoimgextract.cpp)
TARGET_LINK_LIBRARIES(podofoimgextract ${PODOFO_LIB})
SET_TARGET_PROPERTIES(podofoimgextract PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofoimgextract ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofoimgextract
	RUNTIME DESTINATION "bin")
