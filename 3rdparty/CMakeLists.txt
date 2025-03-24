if (MSVC)
    # Hide all warnings for 3rdparty code
	add_compile_options(/W0)
else()
    # Hide all warnings for 3rdparty code
	add_compile_options(-w)

    # Fixes "relocation against hidden symbol can not
    # be used when making a shared object"
    add_compile_options(-fPIC)
endif()

add_compile_definitions(UTF8PROC_STATIC)

add_source("pdfium" "${CMAKE_CURRENT_SOURCE_DIR}/pdfium" SOURCE_FILES HEADERS_FILES)
add_source("afdko" "${CMAKE_CURRENT_SOURCE_DIR}/afdko" SOURCE_FILES HEADERS_FILES)
add_source("utf8proc" "${CMAKE_CURRENT_SOURCE_DIR}/utf8proc" SOURCE_FILES HEADERS_FILES)

list(FILTER SOURCE_FILES EXCLUDE REGEX "utf8proc_data\.c")

include_directories("afdko/resource")
include_directories("afdko/include")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(podofo_3rdparty STATIC ${SOURCE_FILES})
target_link_libraries(podofo_3rdparty LibXml2::LibXml2)
