add_executable(podofopdfinfo podofopdfinfo.cpp pdfinfo.cpp pdfinfo.h)
target_link_libraries(podofopdfinfo
	${PODOFO_LIBRARIES}
	podofo_private
	tools_private
)
install(TARGETS podofopdfinfo RUNTIME DESTINATION "bin")
