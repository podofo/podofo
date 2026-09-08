add_executable(podofotxtextract podofotxtextract.cpp)
target_link_libraries(podofotxtextract tools_private)
install(TARGETS podofotxtextract RUNTIME DESTINATION "bin")
