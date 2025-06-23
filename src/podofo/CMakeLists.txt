if(NOT PODOFO_MAIN_CMAKELISTS_READ)
    message(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
endif()

# Generate our configure file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/auxiliary/podofo_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/podofo/auxiliary/podofo_config.h")

# NOTE: HAS_UNCAUGHT_EXCEPTIONS is needed for <date/date.h>
add_compile_definitions(HAS_UNCAUGHT_EXCEPTIONS=1 UTF8PROC_STATIC)

set(PODOFO_INCLUDE_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/podofo/auxiliary"
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/3rdparty"
    CACHE INTERNAL "podofo include directories"
)

include_directories(
    ${PODOFO_INCLUDE_DIRS}
    ${PODOFO_HEADERS_DEPENDS}
)

add_subdirectory(private)

if (MSVC)
    # Silence linking warnings of podofo_private accessing main podofo symbols
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4217 /ignore:4286")
endif()

set(PODOFO_BASE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/podofo.h")
set(PODOFO_CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/podofo/auxiliary/podofo_config.h")

set(PODOFO_HEADERS ${PODOFO_BASE_HEADER} ${PODOFO_CONFIG_HEADER})
set(PODOFO_SOURCES ${PODOFO_HEADERS})
if(PODOFO_BUILD_SHARED AND WIN32)
    # If we build for windows systems, we also include the resource file
    list(APPEND PODOFO_SOURCES podofo.rc)
endif()

# Set visual studio source group
source_group("" FILES ${PODOFO_SOURCES})

# Add subdirectiories and set for install public headers
foreach(subdir main auxiliary optional staging)
    set(HEADER_FILES)
    add_source(${subdir} "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}" PODOFO_SOURCES HEADER_FILES)
    list(APPEND PODOFO_HEADERS ${HEADER_FILES})
endforeach()

# Set to install further public 3rdparty headers
list(APPEND PODOFO_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/span.hpp")

if(NOT PODOFO_HAVE_FONTCONFIG)
    # Remove FontConfig source if not needed
    list(REMOVE_ITEM PODOFO_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/main/PdfFontConfigWrapper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/main/PdfFontConfigWrapper.cpp"
    )
    list(REMOVE_ITEM PODOFO_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/main/PdfFontConfigWrapper.h"
    )
endif()

add_compile_definitions(PODOFO_BUILD)

if(PODOFO_BUILD_STATIC)
    message("Building static podofo library")
    add_library(podofo_static STATIC ${PODOFO_SOURCES})
    add_library(podofo::podofo ALIAS podofo_static)
    target_link_libraries(podofo_static podofo_private podofo_3rdparty ${PODOFO_LIB_DEPENDS})
    set_target_properties(podofo_static PROPERTIES
        VERSION "${PODOFO_VERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
    )
    set(PODOFO_TARGET podofo_static)
    set(PODOFO_LIBRARIES podofo_static podofo_private podofo_3rdparty
        CACHE INTERNAL "Which podofo library variant to depend on")
    set(PODOFO_PKGCONFIG_LIBS_PRIVATE "-lpodofo_private -lpodofo_3rdparty")
endif()

if(PODOFO_BUILD_SHARED)
    message("Building shared podofo library")
    add_library(podofo_shared SHARED ${PODOFO_SOURCES})
    add_library(podofo::podofo ALIAS podofo_shared)
    target_link_libraries(podofo_shared PRIVATE podofo_private podofo_3rdparty ${PODOFO_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    set_target_properties(podofo_shared PROPERTIES
        VERSION "${PODOFO_VERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
    )
    set(PODOFO_TARGET podofo_shared)
    set(PODOFO_LIBRARIES podofo_shared
        CACHE INTERNAL "Which podofo library variant to depend on")
endif()

# Create a pkg-config file.
# Add a version to the file name corresponding to the API compatibility.
find_program(PKG_CONFIG_FOUND pkg-config pkgconf)
if(PKG_CONFIG_FOUND)
    configure_file(
        "libpodofo.pc.in"
        "${PROJECT_BINARY_DIR}/libpodofo.pc"
        @ONLY)
    install(
        FILES "${PROJECT_BINARY_DIR}/libpodofo.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

# Configure the "headers" file set that will be exported with install(TARGETS)
target_sources(${PODOFO_TARGET} PUBLIC FILE_SET headers TYPE HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/src" ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${PODOFO_HEADERS})

install(TARGETS ${PODOFO_LIBRARIES}
    EXPORT podofo-targets
    FILE_SET headers
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT podofo-targets DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/podofo" NAMESPACE podofo::)

# See https://cmake.org/cmake/help/latest/guide/tutorial/Adding%20Export%20Configuration.html
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/podofo-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/podofo-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/podofo"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/podofo-config-version.cmake"
  VERSION "${PODOFO_VERSION}"
  COMPATIBILITY SameMinorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/podofo-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/podofo-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/podofo"
)

# Use these flags when compiling code that includes podofo headers.
# Failure to do so will result in compilation or link-time errors
# on some platforms, and can even cause undefined results at runtime.
if(PODOFO_BUILD_STATIC)
    set(PODOFO_CFLAGS "-DPODOFO_STATIC" CACHE INTERNAL "Extra flags required when linking to podofo")
else()
    set(PODOFO_CFLAGS "" CACHE INTERNAL "Extra flags required when linking to podofo")
endif()
