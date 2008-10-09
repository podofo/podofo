SET(PODOFOIMPOSE_SOURCES charpainter.cpp pdftranslator.cpp podofoimpose.cpp)

# Add extra sources for Lua-specific code; guard includes of associated
# headers with:
#  #if defined(HAVE_LUA)
# in c++ code.
#
#SET(PODOFOIMPOSE_EXTRA_LIBS)
#IF(LUA_FOUND)
#	SET(PODOFOIMPOSE_SOURCES ${PODOFOIMPOSE_SOURCES} luaplan.cpp)
#       SET(PODOFOIMPOSE_EXTRA_LIBS ${LUA_LIBRARIES})
#	# note that -DHAVE_LUA will also be set
#ENDIF(LUA_FOUND)

ADD_EXECUTABLE(podofoimpose ${PODOFOIMPOSE_SOURCES})
TARGET_LINK_LIBRARIES(podofoimpose ${PODOFO_LIB} ${PODOFOIMPOSE_EXTRA_LIBS})
SET_TARGET_PROPERTIES(podofoimpose PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofoimpose ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofoimpose
	RUNTIME DESTINATION "bin")
