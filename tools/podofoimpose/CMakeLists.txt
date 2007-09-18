ADD_EXECUTABLE(podofoimpose charpainter.cpp pdftranslator.cpp podofoimpose.cpp)
TARGET_LINK_LIBRARIES(podofoimpose ${PODOFO_LIB})
SET_TARGET_PROPERTIES(podofoimpose PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofoimpose ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofoimpose
	RUNTIME DESTINATION "bin")
