set(SOURCE_FILES
    charpainter.h
    charpainter.cpp
    impositionplan.h
	impositionplan.cpp
    pdftranslator.h
	pdftranslator.cpp
    planreader_legacy.h
	planreader_legacy.cpp
	podofoimpose.cpp
)

set(IMPOSE_EXTRA_LIBS)
if(LUA_FOUND)
	set(IMPOSE_EXTRA_LIBS ${LUA_LIBRARIES})
	set(SOURCE_FILES ${SOURCE_FILES} lua_compat.h planreader_lua.h planreader_lua.cpp)
endif()

add_executable(podofoimpose ${SOURCE_FILES})
target_link_libraries(podofoimpose
	${PODOFO_LIBRARIES}
	podofo_private
	${IMPOSE_EXTRA_LIBS}
	tools_private
)
install(TARGETS podofoimpose RUNTIME DESTINATION "bin")
