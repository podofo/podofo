IF(WIN32)
  CMAKE_MINIMUM_REQUIRED(VERSION 2.5)
ELSE(WIN32)
  CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
ENDIF(WIN32)

PROJECT(PoDoFo)

INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(UsePkgConfig)
# [CMAKE 2.5 ONLY]
#INCLUDE(CheckCXXCompilerFlag)

# Load modules from our source tree too
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# By default and if nothing else prevents us,
# build both shared and static libraries.
IF(NOT DEFINED PODOFO_BUILD_SHARED)
    SET(PODOFO_BUILD_SHARED TRUE)
ENDIF(NOT DEFINED PODOFO_BUILD_SHARED)
IF(NOT DEFINED PODOFO_BUILD_STATIC)
    SET(PODOFO_BUILD_STATIC TRUE)
ENDIF(NOT DEFINED PODOFO_BUILD_STATIC)

# Some 64 bit linux distros use /usr/lib64 for 64 bit libraries.
# on these platforms we must
IF(NOT DEFINED WANT_LIB64)
	# TODO: detect 64-bit build and existance of /usr/lib64 and set to TRUE.
	MESSAGE("WANT_LIB64 unset; assuming normal library directory names")
	SET(WANT_LIB64 FALSE)
ENDIF(NOT DEFINED WANT_LIB64)

IF(WANT_LIB64)
	SET(LIBDIRNAME "lib64")
ELSE(WANT_LIB64)
	SET(LIBDIRNAME "lib")
ENDIF(WANT_LIB64)
MESSAGE("Will install libraries to ${CMAKE_INSTALL_PREFIX}/${LIBDIRNAME}")

# CMake 2.5 added the CLEAN_DIRECT_OUTPUT property that we require
# to generate both a shared and static library in the same directory.
# On older versons we can can build EITHER a shared OR static library
# only.
IF(CMAKE_MAJOR_VERSION EQUAL "2" AND CMAKE_MINOR_VERSION LESS "5")
    IF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
         MESSAGE("Both PODOFO_BUILD_SHARED and PODOFO_BUILD_STATIC set")
         MESSAGE("  so disabling shared library generation (CMake version < 2.5)")
         SET(PODOFO_BUILD_SHARED FALSE)
         SET(PODOFO_BUILD_STATIC TRUE)
    ENDIF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
ENDIF(CMAKE_MAJOR_VERSION EQUAL "2" AND CMAKE_MINOR_VERSION LESS "5")

# Linux packagers want an uninstall target.
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}"
    -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

IF(WIN32)
    # On win32 we support EITHER shared OR static builds.
    # If both are enabled (default), turn off generation of the
    # static library.
    IF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
         MESSAGE("Both PODOFO_BUILD_SHARED and PODOFO_BUILD_STATIC set")
         MESSAGE("  so disabling shared library generation (win32)")
         SET(PODOFO_BUILD_SHARED FALSE)
         SET(PODOFO_BUILD_STATIC TRUE)
    ENDIF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
    # We must explicitly link to the core win32 libraries, and we need winsock2
    # to get some byte-order conversion routines too.
    SET(PLATFORM_SYSTEM_LIBRARIES kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid WS2_32)
    # Microsoft deprecate certain POSIX functions that we use.
    # for now, turn off these warnings.
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
    # We need a fake unistd.h for some libraries to build. They try to include <unistd.h>
    # which is not available under win32 with MSVC++, but everything in unistd.h is defined,
    # so an empty file solves the issue.
    SET(EXTRA_INCLUDES vcincludes)
ELSE(WIN32)
    SET(PLATFORM_SYSTEM_LIBRARIES)
    SET(EXTRA_INCLUDES)
ENDIF(WIN32)

IF(UNIX)
    SET(WANT_FONTCONFIG TRUE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
ELSE(UNIX)
    SET(WANT_FONTCONFIG FALSE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
ENDIF(UNIX)


IF(CMAKE_COMPILER_IS_GNUCXX)
    MESSAGE("Using gcc specific compiler options")
    # We can be more specific about what we want out of g++
    # than with most other compilers.

    # Attempt to detect the gcc version. You must not rely on
    # this; use the detected version as a guide only.
    EXEC_PROGRAM(gcc ARGS --version OUTPUT_VARIABLE GCC_VERSION)
    IF(GCC_VERSION MATCHES ".*\\(GCC\\) 4\\.[0-9].*")
        MESSAGE("Have gcc 4.x")
        SET(HAVE_GCC4 1)
    ELSE(GCC_VERSION MATCHES ".*\\(GCC\\) 4\\.[0-9].*")
        SET(HAVE_GCC4 0)
    ENDIF(GCC_VERSION MATCHES ".*\\(GCC\\) 4\\.[0-9].*")

    # If the user hasn't specifically said whether they want
    # -fvisibility=hidden or not, turn it on for gcc4 and off
    # for other gcc versions.
    IF(NOT DEFINED PODOFO_USE_VISIBILITY)
        SET(PODOFO_USE_VISIBILITY ${HAVE_GCC4})
    ENDIF(NOT DEFINED PODOFO_USE_VISIBILITY)

    # We can't request C++98 compliance on win32, because mingw
    # will hide _tzset() and _timezone, which we need for PdfDate.
    # At least until that code is replaced with something better,
    # just don't enable it on win32.
    IF(NOT WIN32)
        ADD_DEFINITIONS(-std=c++98)
    ENDIF(NOT WIN32)

    ADD_DEFINITIONS(
        -Wall
        -Woverloaded-virtual
        -Wswitch-enum
        -Wfloat-equal
        -Wcast-qual
        -Wwrite-strings
        -Wredundant-decls
        -Wreorder
        )
    # STLPort uses old style casts extensively, so if we're building against it
    # we'd best disable the warning.
    IF(NOT USE_STLPORT)
        ADD_DEFINITIONS(-Wold-style-cast)
    ENDIF(NOT USE_STLPORT)
    #
    # Note that we do not need debug definitions here. Set
    # -DCMAKE_BUILD_TYPE=debug or (if you want an optimised
    # release build with debug info) -DCMAKE_CXX_FLAGS="-g3"
    #
    # We add -W unless we're using gcc on win32, where it produces
    # spurious warnings about dllimport of inlines because of a dllimport
    # declaration on the whole class.
    IF(NOT WIN32)
        ADD_DEFINITIONS(-W)
    ENDIF(NOT WIN32)
    # If they've enabled the use of gcc4 symbol visibility, use it.
    IF(PODOFO_USE_VISIBILITY)
        ADD_DEFINITIONS(
            -DHAVE_GCC_SYMBOL_VISIBILITY
            -fvisibility=hidden
            )
    ENDIF(PODOFO_USE_VISIBILITY)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

FIND_PACKAGE(ZLIB REQUIRED)
MESSAGE("Found zlib headers in ${ZLIB_INCLUDE_DIR}, library at ${ZLIB_LIBRARIES}")

FIND_PACKAGE(LIBJPEG)

IF(LIBJPEG_FOUND)
  MESSAGE("Found libjpeg headers in ${LIBJPEG_INCLUDE_DIR}, library at ${LIBJPEG_LIBRARIES}")
  ADD_DEFINITIONS(-DPODOFO_HAVE_JPEG_LIB)
  INCLUDE_DIRECTORIES(${LIBJPEG_INCLUDE_DIR})
ELSE(LIBJPEG_FOUND)
  MESSAGE("Libjpeg not found. JPEG support will be disabled")
ENDIF(LIBJPEG_FOUND)

FIND_PACKAGE(TIFF)

IF(TIFF_FOUND)
  MESSAGE("Found libtiff headers in ${TIFF_INCLUDE_DIR}, library at ${TIFF_LIBRARIES}")
  ADD_DEFINITIONS(-DPODOFO_HAVE_TIFF_LIB)
  INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
ELSE(TIFF_FOUND)
  MESSAGE("Libtiff not found. TIFF support will be disabled")
ENDIF(TIFF_FOUND)

FIND_PACKAGE(FREETYPE REQUIRED)
MESSAGE("Found freetype library at ${FREETYPE_LIBRARIES}, headers ${FREETYPE_INCLUDE_DIR}")

FIND_PACKAGE(LIBSTLPORT)
SET(stlport_libraries_if_use_stlport)
IF(USE_STLPORT)
	IF(LIBSTLPORT_FOUND)
		MESSAGE("Using STLPort")
		INCLUDE_DIRECTORIES(${LIBSTLPORT_HEADERS})
		LINK_DIRECTORIES(${LIBSTLPORT_LIB})
		SET(stlport_libraries_if_use_stlport stlport)
		# Use the threaded STLPort
		ADD_DEFINITIONS(-D_PTHREADS)
	ELSE(LIBSTLPORT_FOUND)
		MESSAGE(FATAL_ERROR "STLPort use requested, but STLPort not found.")
	ENDIF(LIBSTLPORT_FOUND)
ENDIF(USE_STLPORT)

IF(WANT_FONTCONFIG)
	FIND_PACKAGE(LIBFONTCONFIG REQUIRED)
	ADD_DEFINITIONS(-DHAVE_FONTCONFIG)
	SET(PODOFO_LIB_FONTCONFIG:STRING fontconfig)
ELSE(WANT_FONTCONFIG)
	# Might as well look for it anyway. This also sets the appropriate
	# variables to empty values.
	FIND_PACKAGE(LIBFONTCONFIG)
	SET(PODOFO_LIB_FONTCONFIG:STRING)
ENDIF(WANT_FONTCONFIG)

# libjpeg has routines that take a FILE*. This is safe if and only
# if the C runtime podofo is built against is the same as the C runtime
# libjpeg is built against. That is not always the case.
#
# If the user has explicitly told us that libjpeg's libc is binary compatible,
# don't worry about using workarounds. Otherwise, on Windows we assume it's not
# compatible and on other platforms we'll assume it is. This change currently
# only affects PdfImage.cpp.
#
IF(DEFINED JPEG_RUNTIME_COMPATIBLE)
	IF(JPEG_RUNTIME_COMPATIBLE)
		# Trust the user to know what they're doing and pass a FILE*
		ADD_DEFINITIONS(-DPODOFO_JPEG_RUNTIME_COMPATIBLE)
	ENDIF(JPEG_RUNTIME_COMPATIBLE)
ELSE(DEFINED JPEG_RUNTIME_COMPATIBLE)
	IF(NOT WIN32)
		# It's a sensible platform and the user hasn't told us
		# otherwise - pass a FILE* .
		ADD_DEFINITIONS(-DPODOFO_JPEG_RUNTIME_COMPATIBLE)
	ENDIF(NOT WIN32)
ENDIF(DEFINED JPEG_RUNTIME_COMPATIBLE)

IF (WIN32 OR JPEG_RUNTIME_COMPATIBLE)
ELSE (WIN32 OR JPEG_RUNTIME_COMPATIBLE)
ENDIF (WIN32 OR JPEG_RUNTIME_COMPATIBLE)

INCLUDE_DIRECTORIES(
    src
    ${FREETYPE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${EXTRA_INCLUDES}
     )
LINK_DIRECTORIES(
    ${PoDoFo_BINARY_DIR}/src
    )

#
# The PoDoFo library needs to be linked to these libraries,
# as do any apps or libraries linking to PoDoFo. PODOFO_LIB
# will include these and the correct podofo target, so clients
# should specify only PODOFO_LIB .
#
SET(PODOFO_LIB_DEPENDS
    ${ZLIB_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${LIBJPEG_LIBRARIES}
    ${TIFF_LIBRARIES}
    ${PLATFORM_SYSTEM_LIBRARIES}
    ${stlport_libraries_if_use_stlport}
    )

IF(LIBFONTCONFIG_FOUND AND WANT_FONTCONFIG)
	SET(PODOFO_LIB_DEPENDS ${LIBFONTCONFIG_LIB} ${PODOFO_LIB_DEPENDS})
	INCLUDE_DIRECTORIES(${LIBFONTCONFIG_H})
ENDIF(LIBFONTCONFIG_FOUND AND WANT_FONTCONFIG)


SET(PODOFO_LIB
    podofo
    ${PODOFO_LIB_DEPENDS}
    )

# Create the config file. It'll be appended to as the subdirs run though
# then dependency information will be written to it at the end of the
# build.
FILE(WRITE
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "# CMake module for PoDoFo\n"
     )
FILE(APPEND 
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "SET(PODOFO_INCLUDES ${PoDoFo_SOURCE_DIR}/src)\n"
     )

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(examples)

# Export some variables into the config file so it's easier for others
# to build and link against PoDoFo

# To use these dependencies set PODOFO_DIR to the podofo BUILD directory in
# your build (eg -DPODOFO_DIR=/path/to/podofo when running cmake to configure
# the app that'll use podofo). See: FIND_PACKAGE(...) in the cmake docs.
EXPORT_LIBRARY_DEPENDENCIES(
	"${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake"
	APPEND)
