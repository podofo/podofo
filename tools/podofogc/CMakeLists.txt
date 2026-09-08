add_executable(podofogc podofogc.cpp)
target_link_libraries(podofogc tools_private)
install(TARGETS podofogc RUNTIME DESTINATION "bin")
