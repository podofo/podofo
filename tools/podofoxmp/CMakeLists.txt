add_executable(podofoxmp podofoxmp.cpp)
target_link_libraries(podofoxmp ${PODOFO_LIBRARIES})
install(TARGETS podofoxmp RUNTIME DESTINATION "bin")
