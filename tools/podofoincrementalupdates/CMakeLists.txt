add_executable(podofoincrementalupdates incrementalupdates.cpp)
target_link_libraries(podofoincrementalupdates ${PODOFO_LIBRARIES})
install(TARGETS podofoincrementalupdates RUNTIME DESTINATION "bin")
