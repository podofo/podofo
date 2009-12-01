ADD_EXECUTABLE(podofoincrementalupdates incrementalupdates.cpp)

TARGET_LINK_LIBRARIES(podofoincrementalupdates ${PODOFO_LIB})
SET_TARGET_PROPERTIES(podofoincrementalupdates PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofoincrementalupdates ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofoincrementalupdates
	RUNTIME DESTINATION "bin")
