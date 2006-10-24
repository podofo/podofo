CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
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

# CMake 2.5 added the CLEAN_DIRECT_OUTPUT property that we require
# to generate both a shared and static library in the same directory.
# On older versons we can can build EITHER a shared OR static library
# only.
IF(CMAKE_MAJOR_VERSION EQUAL "2" AND CMAKE_MINOR_VERSION LESS "5")
    IF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
         MESSAGE("Both PODOFO_BUILD_SHARED and PODOFO_BUILD_STATIC set")
         MESSAGE("  so disabling static library generation (CMake version < 2.5)")
         SET(PODOFO_BUILD_STATIC FALSE)
    ENDIF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
    
ENDIF(CMAKE_MAJOR_VERSION EQUAL "2" AND CMAKE_MINOR_VERSION LESS "5")

IF(WIN32)
    # On win32 we support EITHER shared OR static builds.
    # If both are enabled (default), turn off generation of the
    # static library.
    IF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
         MESSAGE("Both PODOFO_BUILD_SHARED and PODOFO_BUILD_STATIC set")
         MESSAGE("  so disabling static library generation (win32)")
         SET(PODOFO_BUILD_STATIC FALSE)
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
    ADD_DEFINITIONS(
        -std=c++98
        -Wall
        -Wold-style-cast
        -Woverloaded-virtual
        -Wswitch-enum
        -Wfloat-equal
        -Wcast-qual
        -Wwrite-strings
        -Wredundant-decls
        )
    # We add -W unless we're using gcc on win32, where it produces
    # spurious warnings about dllimport of inlines because of a dllimport
    # declaration on the whole class.
    IF(NOT WIN32)
        ADD_DEFINITIONS(-W)
    ENDIF(NOT WIN32)
    # gcc 4:
    # [CMAKE 2.5 ONLY]
    #CHECK_CXX_COMPILER_FLAG("-fvisibility=hidden" HAVE_GCC4_FLAG_VISIBILITY)
    #IF(HAVE_GCC4_FLAG_VISIBILITY)
    #    ADD_DEFINITIONS("-fvisibility=hidden")
    #ENDIF(HAVE_GCC4_FLAG_VISIBILITY)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

FIND_PACKAGE(ZLIB REQUIRED)
MESSAGE("Found zlib headers in ${LIBZ_H}, library at ${LIBZ_LIB}")

FIND_PACKAGE(LIBJPEG REQUIRED)
MESSAGE("Found libjpeg headers in ${LIBJPEG_H}, library at ${LIBJPEG_LIB}")

FIND_PACKAGE(LIBFREETYPE REQUIRED)
MESSAGE("Found freetype library at ${LIBFREETYPE_LIB}")

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

INCLUDE_DIRECTORIES(
    src
    ${LIBFREETYPE_FT2BUILD_H}
    ${LIBFREETYPE_FREETYPE_H}
    ${LIBZ_H}
    ${LIBJPEG_H}
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
    ${LIBZ_LIB}
    ${LIBFREETYPE_LIB}
    ${LIBJPEG_LIB}
    ${PLATFORM_SYSTEM_LIBRARIES}
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

# Export some variables into the config file so it's easier for others
# to build and link against PoDoFo

# To use these dependencies set PODOFO_DIR to the podofo BUILD directory in
# your build (eg -DPODOFO_DIR=/path/to/podofo when running cmake to configure
# the app that'll use podofo). See: FIND_PACKAGE(...) in the cmake docs.
EXPORT_LIBRARY_DEPENDENCIES(
	"${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake"
	APPEND)
