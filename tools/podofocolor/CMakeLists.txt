set(color_srcs
  podofocolor.cpp 
  colorchanger.cpp 
  colorspace.cpp
  graphicsstack.cpp
  iconverter.cpp
  dummyconverter.cpp
  grayscaleconverter.cpp
)

set(color_extra_libs)

if(PODOFO_HAVE_LUA)
  set(color_extra_libs ${LUA_LIBRARIES})
  set(color_srcs ${color_srcs} luaconverter.cpp)
endif()

add_executable(podofocolor ${color_srcs})

target_link_libraries(podofocolor 
  ${color_extra_libs}
  tools_private
)

install(TARGETS podofocolor RUNTIME DESTINATION "bin")
