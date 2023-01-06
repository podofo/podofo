add_executable(podofosign podofosign.cpp)
include_directories(${OPENSSL_INCLUDE_DIR})

set (PLATFORM_LIBRARIES)
if (WIN32)
    set (PLATFORM_LIBRARIES Ws2_32)
endif()

target_link_libraries(podofosign
	${PODOFO_LIBRARIES}
	podofo_private
    ${OPENSSL_CRYPTO_LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

install(TARGETS podofosign RUNTIME DESTINATION "bin")
