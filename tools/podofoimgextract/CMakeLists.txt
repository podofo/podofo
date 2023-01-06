add_executable(podofoimgextract podofoimgextract.cpp ImageExtractor.cpp ImageExtractor.h)
target_link_libraries(podofoimgextract ${PODOFO_LIBRARIES})
install(TARGETS podofoimgextract RUNTIME DESTINATION "bin")
