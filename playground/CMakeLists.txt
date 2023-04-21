cmake_minimum_required(VERSION 3.16)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Clean CMAKE_CONFIGURATION_TYPES. Only one configuration will be used
unset(CMAKE_CONFIGURATION_TYPES CACHE) # It will also remove it from GUI
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")

project(podofo)
enable_testing() # Needed to enable "make test" from binary dir

# An useful variable to check if we are running from playground
set(PODOFO_PLAYGROUND TRUE)

if ("${ARCH}" STREQUAL "")
    message(FATAL_ERROR "ARCH is not set. Run the boostrap scripts")
endif()

if ("${PODOFO_DIR}" STREQUAL "")
    get_filename_component(PODOFO_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
endif()

set(DEPS_PATH "$ENV{PODOFO_DEPS_PATH}")
if ("${DEPS_PATH}" STREQUAL "")
    set(DEPS_PATH "${PODOFO_DIR}/extern/deps/3rdparty")
endif()

if(NOT EXISTS "${DEPS_PATH}")
    message(FATAL_ERROR "Deps path doesn't exists: try fetch git submodules with:\n    git submodule update --init")
endif()

if (MSVC)
    # Ignore warnings about missing pdbs
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")
endif()

message("ARCHITECTURE: ${ARCH}")
message("PODOFO_DIR: ${PODOFO_DIR}")
message("DEPS_PATH: ${DEPS_PATH}")

# Set include folders for find_package()
list(APPEND CMAKE_INCLUDE_PATH
    "${DEPS_PATH}/zlib/include"
    "${DEPS_PATH}/libxml2/include"
    "${DEPS_PATH}/fontconfig/include"
    "${DEPS_PATH}/freetype/include"
    "${DEPS_PATH}/libpng/include"
    "${DEPS_PATH}/libjpeg/include"
    "${DEPS_PATH}/libtiff/include"
    "${DEPS_PATH}/openssl/include"
    "${DEPS_PATH}/libidn/include"
)

# Set library folders for find_package()
list(APPEND CMAKE_LIBRARY_PATH
    "${DEPS_PATH}/zlib/lib/${ARCH}"
    "${DEPS_PATH}/libxml2/lib/${ARCH}"
    "${DEPS_PATH}/freetype/lib/${ARCH}"
    "${DEPS_PATH}/fontconfig/lib/${ARCH}"
    "${DEPS_PATH}/libpng/lib/${ARCH}"
    "${DEPS_PATH}/libjpeg/lib/${ARCH}"
    "${DEPS_PATH}/libtiff/lib/${ARCH}"
    "${DEPS_PATH}/openssl/lib/${ARCH}"
    "${DEPS_PATH}/libidn/lib/${ARCH}"
)

# Set use libxml2 static for fontconfig
add_compile_definitions(LIBXML_STATIC)
# Set use libidn static
add_compile_definitions(LIBIDN_STATIC)

if(WIN32)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(LIB_SUFFIX "d")
    endif()

    # Following libraries find-modules have issues in correctly
    # determining the right optimized/debug library so we do it manually

    # OpenSSL
    set(OPENSSL_INCLUDE_DIR "${DEPS_PATH}/openssl/include" CACHE PATH "" FORCE)
    set(LIB_EAY_DEBUG "${DEPS_PATH}/openssl/lib/${ARCH}/cryptod.lib" CACHE FILEPATH "" FORCE)
    set(LIB_EAY_RELEASE "${DEPS_PATH}/openssl/lib/${ARCH}/crypto.lib" CACHE FILEPATH "" FORCE)
    set(SSL_EAY_DEBUG "${DEPS_PATH}/openssl/lib/${ARCH}/ssld.lib" CACHE FILEPATH "" FORCE)
    set(SSL_EAY_RELEASE "${DEPS_PATH}/openssl/lib/${ARCH}/ssl.lib" CACHE FILEPATH "" FORCE)
    # Libxml2
    set(LIBXML2_LIBRARY "${DEPS_PATH}/libxml2/lib/${ARCH}/libxml2${LIB_SUFFIX}.lib" CACHE FILEPATH "" FORCE)
    # Fontconfig
    set(Fontconfig_LIBRARY "${DEPS_PATH}/fontconfig/lib/${ARCH}/fontconfig${LIB_SUFFIX}.lib" CACHE FILEPATH "" FORCE)
else()
    # Freetype
    set(FREETYPE_INCLUDE_DIR_ft2build "${DEPS_PATH}/freetype/include/freetype2" CACHE PATH "" FORCE)
    set(FREETYPE_INCLUDE_DIR_freetype2 "${DEPS_PATH}/freetype/include" CACHE PATH "" FORCE)
    set(FREETYPE_LIBRARY_RELEASE "${DEPS_PATH}/freetype/lib/${ARCH}/libfreetype.a" CACHE FILEPATH "" FORCE)
    # Fontconfig
    set(Fontconfig_LIBRARY "${DEPS_PATH}/fontconfig/lib/${ARCH}/libfontconfig.a" CACHE FILEPATH "" FORCE)
    set(PLATFORM_SYSTEM_LIBRARIES "${DEPS_PATH}/bzip2/lib/${ARCH}/libbz2.a" "${DEPS_PATH}/libuuid/lib/${ARCH}/libuuid.a")
endif()

add_subdirectory("${PODOFO_DIR}" "${CMAKE_BINARY_DIR}/podofo")
