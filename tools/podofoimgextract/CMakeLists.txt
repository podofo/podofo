add_executable(podofoimgextract podofoimgextract.cpp ImageExtractor.cpp ImageExtractor.h)
target_link_libraries(podofoimgextract ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofoimgextract RUNTIME DESTINATION "bin")
