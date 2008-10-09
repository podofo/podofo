SET(PODOFOIMPOSE_SOURCES charpainter.cpp pdftranslator.cpp podofoimpose.cpp)

# Add extra sources for Lua-specific code; guard includes of associated
# headers with:
#  #if defined(HAVE_LUA)
# in c++ code.
#
#IF(LUA_FOUND)
#	SET(PODOFOIMPOSE_SOURCES ${PODOFOIMPOSE_SOURCES} luaplan.cpp)
#	# note that -DHAVE_LUA will also be set
#ENDIF(LUA_FOUND)

ADD_EXECUTABLE(podofoimpose ${PODOFOIMPOSE_SOURCES})
TARGET_LINK_LIBRARIES(podofoimpose ${PODOFO_LIB} ${LUA_LIBRARIES})
SET_TARGET_PROPERTIES(podofoimpose PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofoimpose ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofoimpose
	RUNTIME DESTINATION "bin")
