# Hide all warnings for 3rdparty code
if (MSVC)
	add_compile_options(/W0)
else()
	add_compile_options(-w)
endif()

add_source("pdfium" "${PROJECT_SOURCE_DIR}/3rdparty/pdfium" SOURCE_FILES HEADERS_FILES)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(podofo_3rdparty STATIC ${SOURCE_FILES})
