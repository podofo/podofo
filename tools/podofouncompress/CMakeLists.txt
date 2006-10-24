ADD_EXECUTABLE(podofouncompress Uncompress.cpp podofouncompress.cpp)
TARGET_LINK_LIBRARIES(podofouncompress ${PODOFO_LIB})
SET_TARGET_PROPERTIES(podofouncompress PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofouncompress ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofouncompress
	RUNTIME DESTINATION "bin")
