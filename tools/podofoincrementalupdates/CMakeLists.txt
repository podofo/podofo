add_executable(podofoincrementalupdates incrementalupdates.cpp)
target_link_libraries(podofoincrementalupdates ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofoincrementalupdates RUNTIME DESTINATION "bin")
