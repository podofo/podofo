add_executable(podofoencrypt podofoencrypt.cpp)
target_link_libraries(podofoencrypt ${PODOFO_LIBRARIES})
install(TARGETS podofoencrypt RUNTIME DESTINATION "bin")
