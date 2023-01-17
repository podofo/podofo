add_executable(podofosign podofosign.cpp)
include_directories(${OPENSSL_INCLUDE_DIR})

set (PLATFORM_LIBRARIES)
if (WIN32)
    set (PLATFORM_LIBRARIES Ws2_32)
endif()

target_link_libraries(podofosign
    ${OPENSSL_CRYPTO_LIBRARIES} # NOTE: For mac linking, this must be first
	${PODOFO_LIBRARIES}
	podofo_private
    ${PLATFORM_LIBRARIES}
)

install(TARGETS podofosign RUNTIME DESTINATION "bin")
