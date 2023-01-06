set(PDF_TEST_RESOURCE_PATH "${PROJECT_SOURCE_DIR}/extern/resources")
if(NOT EXISTS "${PDF_TEST_RESOURCE_PATH}")
    message(WARNING "Test resources path doesn't exists, tests disabled. Try fetch git submodules if you want tests to be enabled")
    return()
endif()

set(PDF_TEST_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/out")
make_directory(${PDF_TEST_OUTPUT_PATH})

include_directories("${PROJECT_SOURCE_DIR}/3rdparty/include")

file(TO_NATIVE_PATH "${PDF_TEST_RESOURCE_PATH}" PDF_TEST_RESOURCE_PATH)
file(TO_NATIVE_PATH "${PDF_TEST_OUTPUT_PATH}" PDF_TEST_OUTPUT_PATH)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/PdfTestConfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/PdfTestConfig.h")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(common)
include_directories(common)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
set(Catch2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")
find_package(Catch2 REQUIRED)

add_subdirectory(unit)
