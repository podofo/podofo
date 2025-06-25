cmake_minimum_required(VERSION 3.23)

project(PoDoFo)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        message(FATAL_ERROR "GCC version must be at least 9.0")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "Clang version must be at least 7.0")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.9")
        message(FATAL_ERROR "MSVC version must be at least 15.9")
    endif()
else()
    message(WARNING "You are using an unsupported compiler!")
endif()

set(PODOFO_VERSION_MAJOR "1" CACHE STRING "Major part of podofo version number")
set(PODOFO_VERSION_MINOR "1" CACHE STRING "Minor part of podofo version number")
set(PODOFO_VERSION_PATCH "0" CACHE STRING "Patchlevel part of podofo version number")
set(PODOFO_VERSION "${PODOFO_VERSION_MAJOR}.${PODOFO_VERSION_MINOR}.${PODOFO_VERSION_PATCH}")
set(PODOFO_SOVERSION "3")

set(DOXYGEN_PROJECT_NUMBER "${PODOFO_VERSION}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY
)

find_package(Doxygen)
if (DOXYGEN_FOUND)
    # TODO: Add doxygen target
endif()

if(PODOFO_BUILD_DOC_ONLY)
    return()
endif()

#
# Main includes
#
include(CheckIncludeFile)
include(CheckLibraryExists)
include(TestBigEndian)
include(CheckTypeSize)
include(GNUInstallDirs)

# Load modules from our source tree too
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" "${CMAKE_CURRENT_BINARY_DIR}")

# Builds must use this CMakeLists.txt, not the one in src/ or somewhere else.
# If users try to use something else the results can be confusing. We set a
# variable here that we require to be set elsewhere, otherwise we'll complain.
set(PODOFO_MAIN_CMAKELISTS_READ TRUE)

if(PODOFO_BUILD_LIB_ONLY)
    set(PODOFO_BUILD_TEST FALSE)
    set(PODOFO_BUILD_EXAMPLES FALSE)
    set(PODOFO_BUILD_UNSUPPORTED_TOOLS FALSE)
else()
    if (NOT DEFINED PODOFO_BUILD_TEST)
        set(PODOFO_BUILD_TEST TRUE)
    endif()

    if (NOT DEFINED PODOFO_BUILD_EXAMPLES)
        set(PODOFO_BUILD_EXAMPLES TRUE)
    endif()

    if (NOT DEFINED PODOFO_BUILD_UNSUPPORTED_TOOLS)
        set(PODOFO_BUILD_UNSUPPORTED_TOOLS FALSE)
    endif()

    # We assume a standalone build so we set output
    # path to a fixed location
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
    message(STATUS "Binary output directory is \"${CMAKE_BINARY_DIR}/target\"")
endif()

if (DEFINED PODOFO_BUILD_SHARED)
    message(FATAL_ERROR "PODOFO_BUILD_SHARED shouldn't be defined externally. Set PODOFO_BUILD_STATIC to TRUE if you need a static build")
endif()

option(PODOFO_BUILD_STATIC "Build static libraries (.a/.lib)" FALSE)
if(PODOFO_BUILD_STATIC)
    set(PODOFO_BUILD_SHARED FALSE)
    add_compile_definitions(PODOFO_STATIC)
else()
    set(PODOFO_BUILD_SHARED TRUE)
    add_compile_definitions(PODOFO_SHARED)
endif()

message("Will install libraries to ${CMAKE_INSTALL_FULL_LIBDIR}")

# Linux packagers want an uninstall target.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}"
    -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

if(NOT DEFINED PLATFORM_SYSTEM_LIBRARIES)
    set(PLATFORM_SYSTEM_LIBRARIES)
endif()

if(WIN32)
    # Core win32 libraries come with CMAKE_C(XX)_IMPLICIT_LINK_LIBRARIES
    list(APPEND PLATFORM_SYSTEM_LIBRARIES ws2_32 crypt32)
endif()

# Set minimum required standard C++17, but also enforce it
# with compiler options as CMAKE_CXX_STANDARD (or any other
# CMake construct) won't really do it.
# See https://discourse.cmake.org/t/how-to-limit-maximum-cxx-standard-to-prevent-accidental-c-17-or-above-code/9862/2
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if (MSVC)
    # Set the compiler less permissive and add some warnings
    # /we4287: unsigned/negative constant mismatch
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++17> $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus> /permissive- /utf-8 /we4287 /W4)

    # Microsoft deprecate certain POSIX functions that we use.
    # for now, turn off these warnings.
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
else()
    # By default hide symbols on gcc/clang
    # TODO: Remove -Wno-format-truncation after porting the
    # code to std::fmt as it will be unuseful
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17> $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual> $<$<COMPILE_LANGUAGE:CXX>:-Wreorder> -fvisibility=hidden -Wall -Wswitch -Wcast-qual -Wwrite-strings -Wredundant-decls -Wno-format-truncation -Wno-unknown-pragmas -Wno-unknown-warning-option)

    # Note that we do not need debug definitions here. Set
    # -DCMAKE_BUILD_TYPE=debug or (if you want an optimised
    # release build with debug info) -DCMAKE_CXX_FLAGS="-g3"
    #
    # We add -W unless we're using gcc on win32, where it produces
    # spurious warnings about dllimport of inlines because of a dllimport
    # declaration on the whole class.
    if(NOT WIN32)
        add_definitions(-W)
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # Silence warnings on use of typeid(*ptr)
        # See https://stackoverflow.com/questions/46494928/clang-warning-on-expression-side-effects
        add_compile_options(-Wno-potentially-evaluated-expression)
    endif()
endif()

find_package(ZLIB REQUIRED)
message("Found zlib headers in ${ZLIB_INCLUDE_DIR}, library at ${ZLIB_LIBRARIES}")

find_package(OpenSSL REQUIRED)
message("OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

if (PODOFO_WANT_LCMS2)
find_package(LCMS2)
if(LCMS2_FOUND)
    message("Found little-cms2 headers in ${LCMS2_INCLUDE_DIR}, library at ${LCMS2_LIBRARIES}")
    set(PODOFO_HAVE_LCMS2 TRUE)
    message("little-cms2 found. ICC profile reading support will be enabled")
else()
    message("little-cms2 not found. ICC profile reading support will be disabled")
endif()
endif()

find_package(JPEG)

if(JPEG_FOUND)
    message("Found libjpeg headers in ${JPEG_INCLUDE_DIR}, library at ${JPEG_LIBRARIES}")
    set(PODOFO_HAVE_JPEG_LIB TRUE)
else()
    message("Libjpeg not found. JPEG support will be disabled")
endif()

find_package(TIFF)

if(TIFF_FOUND)
    message("Found libtiff headers in ${TIFF_INCLUDE_DIR}, library at ${TIFF_LIBRARIES}")
    set(PODOFO_HAVE_TIFF_LIB TRUE)
else()
    message("Libtiff not found. TIFF support will be disabled")
endif()

find_package(PNG)

if(PNG_FOUND)
    message("Found LibPng headers in ${PNG_INCLUDE_DIR}, library at ${PNG_LIBRARIES}")
    set(PODOFO_HAVE_PNG_LIB TRUE)
else()
    message("LibPng not found. PNG support will be disabled")
    set(PNG_LIBRARIES "")
endif()

find_package(Freetype REQUIRED)
message("Found freetype library at ${FREETYPE_LIBRARIES}, headers ${FREETYPE_INCLUDE_DIRS}")

find_package(Fontconfig)
if(Fontconfig_FOUND)
    set(PODOFO_HAVE_FONTCONFIG TRUE)
    message("Found fontconfig headers in ${Fontconfig_INCLUDE_DIRS}, library at ${Fontconfig_LIBRARIES}")
endif()

if(WIN32 AND NOT PODOFO_HAVE_FONTCONFIG OR PODOFO_WANT_WIN32GDI)
    # Fallback to GDI if FontConfig is missing in Windows
    set(PODOFO_HAVE_WIN32GDI TRUE)
    message("Enabled Windows GDI API")
endif()

find_package(LibXml2 REQUIRED)
message("Found libxml2 library at ${LIBXML2_LIBRARIES}, headers ${LIBXML2_INCLUDE_DIRS}")

# The podofo library needs to be linked to these libraries
# NOTE: Be careful when adding/removing: the order may be
# platform sensible, so don't modify the current order

set(PODOFO_PKGCONFIG_REQUIRES_PRIVATE "")
set(PODOFO_HEADERS_DEPENDS)
set(PODOFO_LIB_DEPENDS OpenSSL::SSL)
string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " libssl")
if(Fontconfig_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS Fontconfig::Fontconfig)
    string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " fontconfig")
endif()
list(APPEND PODOFO_LIB_DEPENDS Freetype::Freetype)
string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " freetype2")
list(APPEND PODOFO_LIB_DEPENDS LibXml2::LibXml2)
string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " libxml-2.0")
if(PNG_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS PNG::PNG)
    string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " libpng")
endif()
if(TIFF_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS TIFF::TIFF)
    string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " libtiff-4")
endif()
if(JPEG_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS JPEG::JPEG)
    string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " libjpeg")
endif()
list(APPEND PODOFO_LIB_DEPENDS ZLIB::ZLIB)
string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " zlib")
list(APPEND PODOFO_LIB_DEPENDS ${PLATFORM_SYSTEM_LIBRARIES})

if(LCMS2_FOUND)
    # little-cms2 doesn't provide targets.
    list(APPEND PODOFO_LIB_DEPENDS ${LCMS2_LIBRARIES})
    list(APPEND PODOFO_HEADERS_DEPENDS ${LCMS2_INCLUDE_DIR})
    string(APPEND PODOFO_PKGCONFIG_REQUIRES_PRIVATE " lcms2")
endif()

function(add_source subdir subdir_path dstsources dstheaders)
    file(GLOB_RECURSE source_files "${subdir_path}/[^.]*")
    # Set visual studio source group
    source_group(${subdir} FILES ${source_files})
    # Add subdir to source files
    list(APPEND ${dstsources} ${source_files})
    set(${dstsources} ${${dstsources}} PARENT_SCOPE)

    # Identity the headers and do some cleanup
    set(headers_files ${source_files})
    list(FILTER headers_files EXCLUDE REGEX "\.cpp$")
    list(FILTER headers_files EXCLUDE REGEX "\.in$")
    list(APPEND ${dstheaders} ${headers_files})
    set(${dstheaders} ${${dstheaders}} PARENT_SCOPE)
endfunction()

add_subdirectory(3rdparty)
add_subdirectory(src/podofo)
include_directories(${PODOFO_INCLUDE_DIRS})

if(PODOFO_BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

if(PODOFO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(PODOFO_BUILD_UNSUPPORTED_TOOLS)
    add_subdirectory(tools)
endif()

# Enable packaging
set(CPACK_PACKAGE_DESCRIPTION "A C++ PDF manipulation library")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/podofo/podofo")
set(CPACK_PACKAGE_VENDOR "Francesco Pretto")
set(CPACK_PACKAGE_CONTACT "ceztko@gmail.com")
set(CPACK_PACKAGE_VERSION ${PODOFO_VERSION})
if (WIN32)
    # Set MSI generation
    set(CPACK_GENERATOR "WIX")
endif()
include(CPack)
