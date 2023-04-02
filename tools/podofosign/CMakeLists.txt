add_executable(podofosign podofosign.cpp)
include_directories(${OPENSSL_INCLUDE_DIR})

target_link_libraries(podofosign
    OpenSSL::Crypto # In non MSVC the order matters and this should be first
	${PODOFO_LIBRARIES}
	podofo_private
	tools_private
)

install(TARGETS podofosign RUNTIME DESTINATION "bin")
