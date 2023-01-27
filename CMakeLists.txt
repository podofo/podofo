cmake_minimum_required(VERSION 3.16)

project(PoDoFo)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.1)
        message(FATAL_ERROR "GCC version must be at least 8.1")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "Clang version must be at least 7.0")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.9")
        message(FATAL_ERROR "MSVC version bust be at least 15.9")
    endif()
else()
    message(WARNING "You are using an unsupported compiler!")
endif()

set(PODOFO_VERSION_MAJOR "0" CACHE STRING "Major part of podofo version number")
set(PODOFO_VERSION_MINOR "10" CACHE STRING "Minor part of podofo version number")
set(PODOFO_VERSION_PATCH "0" CACHE STRING "Patchlevel part of podofo version number")
set(PODOFO_SOVERSION "${PODOFO_VERSION_MAJOR}.${PODOFO_VERSION_MINOR}.${PODOFO_VERSION_PATCH}")
set(PODOFO_LIBVERSION "${PODOFO_SOVERSION}")

#
# Main includes
#
include(CheckIncludeFile)
include(CheckLibraryExists)
include(TestBigEndian)
include(CheckTypeSize)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

# Load modules from our source tree too
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" "${CMAKE_CURRENT_BINARY_DIR}")

# Builds must use this CMakeLists.txt, not the one in src/ or somewhere else.
# If users try to use something else the results can be confusing. We set a
# variable here that we require to be set elsewhere, otherwise we'll complain.
set(PODOFO_MAIN_CMAKELISTS_READ TRUE)

if(NOT PODOFO_BUILD_LIB_ONLY)
    # We assume a standalone build so we set output
    # path to a fixed location
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
endif()

option(PODOFO_SHARED "Build shared libraries (.dll/.so)" ON)
if(PODOFO_SHARED)
    add_compile_definitions(PODOFO_SHARED)
else()
    set(PODOFO_STATIC TRUE)
    add_compile_definitions(PODOFO_STATIC)
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
    # We must explicitly link to the core win32 libraries
    list(APPEND PLATFORM_SYSTEM_LIBRARIES kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid ws2_32 Crypt32)
endif()

if (MSVC)
    # Set the compiler less permissive and add some warnings
    # /w34388: 'expression': signed/unsigned mismatch
    # /w34389: 'operator': signed/unsigned mismatch
    # /we4287: unsigned/negative constant mismatch
    # /w34100: unreferenced formal parameter
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive- /w34388 /w34389 /we4287 /w34100 /Zc:__cplusplus")

    # Microsoft deprecate certain POSIX functions that we use.
    # for now, turn off these warnings.
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
else()
    message("Using gcc/clang specific compiler options")

    # By default hide symbols on gcc/clang
    add_compile_options(-fvisibility=hidden)

    # TODO: Remove -Wno-format-truncation after porting the
    # code to std::fmt as it will be unuseful
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wswitch -Wcast-qual -Wwrite-strings -Wredundant-decls -Wreorder -Wno-format-truncation -Wno-unknown-pragmas -Wno-unknown-warning-option")

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
endif()

find_package(ZLIB REQUIRED)
message("Found zlib headers in ${ZLIB_INCLUDE_DIR}, library at ${ZLIB_LIBRARIES}")

find_package(OpenSSL REQUIRED)
message("OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

find_package(Libidn)

if(LIBIDN_FOUND)
    message("Found Libidn headers in ${LIBIDN_INCLUDE_DIR}, library at ${LIBIDN_LIBRARIES}")
    set(PODOFO_HAVE_LIBIDN TRUE)
    message("Libidn found. AES-256 Encryption support will be enabled")
else()
    message("Libidn not found. AES-256 Encryption support will be disabled")
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

set(PODOFO_HEADERS_DEPENDS)
set(PODOFO_LIB_DEPENDS OpenSSL::SSL)
if(Fontconfig_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS Fontconfig::Fontconfig)
endif()
list(APPEND PODOFO_LIB_DEPENDS Fontconfig::Fontconfig)
list(APPEND PODOFO_LIB_DEPENDS Freetype::Freetype)
list(APPEND PODOFO_LIB_DEPENDS LibXml2::LibXml2)
if(PNG_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS PNG::PNG)
endif()
if(TIFF_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS TIFF::TIFF)
endif()
if(JPEG_FOUND)
    list(APPEND PODOFO_LIB_DEPENDS JPEG::JPEG)
endif()
list(APPEND PODOFO_LIB_DEPENDS ZLIB::ZLIB)
list(APPEND PODOFO_LIB_DEPENDS ${PLATFORM_SYSTEM_LIBRARIES})

if(LIBIDN_FOUND)
    # libidn doesn't provide targets.
    list(APPEND PODOFO_LIB_DEPENDS ${LIBIDN_LIBRARIES})
    list(APPEND PODOFO_HEADERS_DEPENDS ${LIBIDN_INCLUDE_DIR})
endif()

# Create the config file. It'll be appended to as the subdirs run though
# then dependency information will be written to it at the end of the
# build.
file(WRITE
    "${PROJECT_BINARY_DIR}/podofoConfig.cmake"
    "# CMake module for podofo\n"
)
file(APPEND
    "${PROJECT_BINARY_DIR}/podofoConfig.cmake"
    "set(PODOFO_INCLUDES ${PROJECT_SOURCE_DIR}/src)\n"
)

add_subdirectory(src/podofo)
include_directories(${PODOFO_INCLUDE_DIRS})

if(NOT PODOFO_BUILD_LIB_ONLY)
    enable_testing()
    add_subdirectory(test)
    add_subdirectory(examples)
    add_subdirectory(tools)
endif()

# Export some variables into the config file so it's easier for others
# to build and link against podofo

# To use these dependencies set PODOFO_DIR to the podofo BUILD directory in
# your build (eg -DPODOFO_DIR=/path/to/podofo when running cmake to configure
# the app that'll use podofo). See: find_package(...) in the cmake docs.
if(PODOFO_SHARED)
    export(TARGETS podofo_shared FILE "${PROJECT_BINARY_DIR}/podofoConfig.cmake")
else()
    export(TARGETS podofo_static podofo_private FILE "${PROJECT_BINARY_DIR}/podofoConfig.cmake")
endif()

# Enable packaging
set(CPACK_PACKAGE_DESCRIPTION "A C++ PDF manipulation library")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/podofo/podofo")
set(CPACK_PACKAGE_VENDOR "Francesco Pretto")
set(CPACK_PACKAGE_CONTACT "ceztko@gmail.com")
set(CPACK_PACKAGE_VERSION ${PODOFO_LIBVERSION})
if (WIN32)
    # Set MSI generation
    set(CPACK_GENERATOR "WIX")
endif()
include(CPack)
