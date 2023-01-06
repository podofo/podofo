add_executable(podofonooc podofonooc.cpp)
target_link_libraries(podofonooc ${PODOFO_LIBRARIES})
install(TARGETS podofonooc RUNTIME DESTINATION "bin")
