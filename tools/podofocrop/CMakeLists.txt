add_executable(podofocrop podofocrop.cpp)
target_link_libraries(podofocrop tools_private)
install(TARGETS podofocrop RUNTIME DESTINATION "bin")
