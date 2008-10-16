# PoDoFoImpose

SET(impose_srcs
	podofoimpose.cpp
	pdftranslator.cpp
	impositionplan.cpp
	planreader_legacy.cpp
)

SET(impose_extra_libs)

IF(LUA_FOUND)
	SET(impose_extra_libs ${LUA_LIBRARIES})
	SET(impose_srcs ${impose_srcs} planreader_lua.cpp)
ENDIF(LUA_FOUND)

ADD_EXECUTABLE(podofoimpose ${impose_srcs} )

TARGET_LINK_LIBRARIES(podofoimpose 
	${PODOFO_LIB}
	${impose_extra_libs}
)

SET_TARGET_PROPERTIES(podofoimpose PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")

ADD_DEPENDENCIES(podofoimpose ${PODOFO_DEPEND_TARGET})

INSTALL(TARGETS podofoimpose
	RUNTIME DESTINATION "bin")
