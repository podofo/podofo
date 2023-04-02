add_executable(podofonooc podofonooc.cpp)
target_link_libraries(podofonooc ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofonooc RUNTIME DESTINATION "bin")
