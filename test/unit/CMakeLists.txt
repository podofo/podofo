file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "[^.]*.h" "[^.]*.cpp")
source_group("" FILES ${SOURCE_FILES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# repeat for each test
add_executable(podofo-unit ${SOURCE_FILES})
# NOTE: OpenSSL, Freetype and Fontconfig headers are reached directly
# by some tests
target_link_libraries(podofo-unit PRIVATE podofo_test OpenSSL::Crypto Freetype::Freetype)
if(Fontconfig_FOUND)
    target_link_libraries(podofo-unit PRIVATE Fontconfig::Fontconfig)
endif()

include(CTest)
include(Catch)
catch_discover_tests(podofo-unit)
