SET(color_srcs
  podofocolor.cpp 
  colorchanger.cpp 
  colorspace.cpp
  graphicsstack.cpp
  iconverter.cpp
  dummyconverter.cpp
  grayscaleconverter.cpp
  )


SET(color_extra_libs)

IF(LUA_FOUND)
  SET(color_extra_libs ${LUA_LIBRARIES})
  SET(color_srcs ${color_srcs} luaconverter.cpp)
ENDIF(LUA_FOUND)

ADD_EXECUTABLE(podofocolor ${color_srcs} )

TARGET_LINK_LIBRARIES(podofocolor 
  ${PODOFO_LIB}
  ${color_extra_libs}
)

SET_TARGET_PROPERTIES(podofocolor PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(podofocolor ${PODOFO_DEPEND_TARGET})
INSTALL(TARGETS podofocolor
	RUNTIME DESTINATION "bin")
