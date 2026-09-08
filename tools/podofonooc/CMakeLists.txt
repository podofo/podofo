add_executable(podofonooc podofonooc.cpp)
target_link_libraries(podofonooc tools_private)
install(TARGETS podofonooc RUNTIME DESTINATION "bin")
