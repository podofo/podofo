add_executable(podofoimg2pdf podofoimg2pdf.cpp ImageConverter.cpp ImageConverter.h)
target_link_libraries(podofoimg2pdf ${PODOFO_LIBRARIES} tools_private)
install(TARGETS podofoimg2pdf RUNTIME DESTINATION "bin")
