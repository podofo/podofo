add_executable(podofoencrypt podofoencrypt.cpp)
target_link_libraries(podofoencrypt ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofoencrypt RUNTIME DESTINATION "bin")
