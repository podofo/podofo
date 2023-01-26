add_executable(podofosign podofosign.cpp)
include_directories(${OPENSSL_INCLUDE_DIR})

target_link_libraries(podofosign
	${PODOFO_LIBRARIES}
	podofo_private
)

install(TARGETS podofosign RUNTIME DESTINATION "bin")
