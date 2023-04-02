add_executable(podofotxt2pdf podofotxt2pdf.cpp)
target_link_libraries(podofotxt2pdf ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofotxt2pdf RUNTIME DESTINATION "bin")
