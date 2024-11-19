if (MSVC)
    # Hide all warnings for 3rdparty code
	add_compile_options(/W0)
else()
    # Hide all warnings for 3rdparty code
	add_compile_options(-w)

    if (PODOFO_BUILD_SHARED)
        # Fixes "relocation against hidden symbol can not
        # be used when making a shared object"
        add_compile_options(-fPIC)
    endif()
endif()

add_source("pdfium" "${PROJECT_SOURCE_DIR}/3rdparty/pdfium" SOURCE_FILES HEADERS_FILES)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(podofo_3rdparty STATIC ${SOURCE_FILES})
