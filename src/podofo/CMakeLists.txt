if(NOT PODOFO_MAIN_CMAKELISTS_READ)
    message(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
endif()

# Generate our configure file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/podofo_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/podofo_config.h")

# Needed for <date/date.h>
add_compile_definitions(HAS_UNCAUGHT_EXCEPTIONS=1)

set(PODOFO_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/3rdparty"
    CACHE INTERNAL "podofo include directories"
)

include_directories(
    ${PODOFO_INCLUDE_DIRS}
    ${PODOFO_HEADERS_DEPENDS}
)

add_subdirectory(private)

set(PODOFO_BASE_HEADERS
    podofo-main.h
    podofo-staging.h
    podofo.h
)
set(PODOFO_CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/podofo_config.h")

install(FILES ${PODOFO_BASE_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/podofo")
install(FILES ${PODOFO_CONFIG_HEADER} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/podofo/main")

set(PODOFO_SOURCES ${PODOFO_BASE_HEADERS} ${PODOFO_CONFIG_HEADER})
if(PODOFO_SHARED AND WIN32)
    # If we build for windows systems, we also include the resource file
    list(APPEND PODOFO_SOURCES podofo.rc)
endif()

# Set visual studio source group
source_group("" FILES ${PODOFO_SOURCES})

macro(add_source subdir subdir_path)
    file(GLOB_RECURSE SOURCE_FILES "${subdir_path}/*")
    # Set visual studio source group
    source_group(${subdir} FILES ${SOURCE_FILES})
    # Add subdir to source files
    list(APPEND PODOFO_SOURCES ${SOURCE_FILES})
    # Set non private headers for install
    set(HEADER_FILES ${SOURCE_FILES})
    list(FILTER HEADER_FILES EXCLUDE REGEX "\.cpp$")
endmacro()

foreach(subdir common main staging)
    add_source(${subdir} "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
    install(FILES ${HEADER_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/podofo/${subdir}")
endforeach()

if(NOT PODOFO_HAVE_FONTCONFIG)
    # Remove FontConfig source if not needed
    list(REMOVE_ITEM PODOFO_SOURCES
        main/PdfFontConfigWrapper.h
        main/PdfFontConfigWrapper.cpp
    )
endif()

add_source("3rdparty/pdfium" "${PROJECT_SOURCE_DIR}/3rdparty/pdfium")

if(PODOFO_STATIC)
    message("Building static podofo library")
    add_library(podofo_static STATIC ${PODOFO_SOURCES})
    target_link_libraries(podofo_static podofo_private ${PODOFO_LIB_DEPENDS})
    set_target_properties(podofo_static PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
    )
    install(TARGETS podofo_static
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    set(PODOFO_LIBRARIES podofo_static podofo_private
        CACHE INTERNAL "Which podofo library variant to depend on")
endif()

if(PODOFO_SHARED)
    message("Building shared podofo library")
    add_library(podofo_shared SHARED ${PODOFO_SOURCES})
    target_link_libraries(podofo_shared PRIVATE podofo_private ${PODOFO_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    set_target_properties(podofo_shared PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
    )
    # Since we're building a shared podofo, prefer to depend on this one for
    # tests and tools over the static library (if built).
    install(TARGETS podofo_shared
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # Create a pkg-config file for linking against shared library
    # if pkg-config is available on the system.
    # Add a version to the file name corresponding to the API compatibility.

    find_program(PKG_CONFIG_FOUND pkg-config)
    if(PKG_CONFIG_FOUND)
        message("Pkg-config found, creating a pkg-config file for linking against shared library.")
        configure_file(
            "libpodofo.pc.in"
            "${PROJECT_BINARY_DIR}/libpodofo.pc"
            @ONLY)
        install(
            FILES "${PROJECT_BINARY_DIR}/libpodofo.pc"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    else()
        message("Pkg-config not found. No pkg-config file will be created.")
    endif()

    set(PODOFO_LIBRARIES podofo_shared
        CACHE INTERNAL "Which podofo library variant to depend on")
endif()

# Use these flags when compiling code that includes podofo headers.
# Failure to do so will result in compilation or link-time errors
# on some platforms, and can even cause undefined results at runtime.
if(STATIC_PODOFO)
    set(PODOFO_CFLAGS "-DPODOFO_STATIC" CACHE INTERNAL "Extra flags required when linking to podofo")
else()
    set(PODOFO_CFLAGS "" CACHE INTERNAL "Extra flags required when linking to podofo")
endif()

# Write the cflags and depend target to the config file
file(APPEND
    "${PROJECT_BINARY_DIR}/podofoConfig.cmake"
    "set(PODOFO_CFLAGS ${PODOFO_CFLAGS})\n"
)
file(APPEND
    "${PROJECT_BINARY_DIR}/podofoConfig.cmake"
    "set(PODOFO_LIBRARIES ${PODOFO_LIBRARIES})\n"
)
