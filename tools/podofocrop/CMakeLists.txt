add_executable(podofocrop podofocrop.cpp)
target_link_libraries(podofocrop ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofocrop RUNTIME DESTINATION "bin")
