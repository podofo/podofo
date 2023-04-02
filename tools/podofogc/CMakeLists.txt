add_executable(podofogc podofogc.cpp)
target_link_libraries(podofogc ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofogc RUNTIME DESTINATION "bin")
