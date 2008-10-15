# PoDoFoImpose

FIND_PACKAGE(Lua51)
IF(LUA51_FOUND)
	ADD_DEFINITIONS(-DHAVE_LUA)
	SET(Lua_Reader planreader_lua.cpp)
	MESSAGE(STATUS "Build with Lua support (51)")
ELSE(LUA51_FOUND)
	FIND_PACKAGE(Lua50)
	IF(LUA50_FOUND)
		ADD_DEFINITIONS(-DHAVE_LUA)
		SET(Lua_Reader planreader_lua.cpp)
		MESSAGE(STATUS "Build with Lua support (50)")
	ENDIF(LUA50_FOUND)
ENDIF(LUA51_FOUND)


SET(impose_extra_libs
${LUA_LIBRARIES}
)

INCLUDE_DIRECTORIES(
${LUA_INCLUDE_DIR}
)

SET(plan_readers 
planreader_legacy.cpp
${Lua_Reader}
)


SET(impose_srcs
podofoimpose.cpp
pdftranslator.cpp
impositionplan.cpp
${plan_readers}
)

ADD_EXECUTABLE(podofoimpose  ${impose_srcs} )

TARGET_LINK_LIBRARIES(podofoimpose 
${PODOFO_LIB}
${impose_extra_libs}
)

SET_TARGET_PROPERTIES(podofoimpose PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")

ADD_DEPENDENCIES(podofoimpose ${PODOFO_DEPEND_TARGET})

INSTALL(TARGETS podofoimpose
	RUNTIME DESTINATION "bin")
