add_executable(podofocrop podofocrop.cpp)
target_link_libraries(podofocrop ${PODOFO_LIBRARIES})
install(TARGETS podofocrop RUNTIME DESTINATION "bin")
