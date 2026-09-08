add_executable(podofomerge podofomerge.cpp)
target_link_libraries(podofomerge tools_private)
install(TARGETS podofomerge RUNTIME DESTINATION "bin")
