CMAKE_MINIMUM_REQUIRED(VERSION 1.5)
PROJECT(PoDoFo)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(UsePkgConfig)
# [CMAKE 2.5 ONLY]
#INCLUDE(CheckCXXCompilerFlag)

IF(NOT DEFINED PODOFO_BUILD_SHARED)
    SET(PODOFO_BUILD_SHARED TRUE)
ENDIF(NOT DEFINED PODOFO_BUILD_SHARED)

IF(NOT DEFINED PODOFO_BUILD_STATIC)
    SET(PODOFO_BUILD_STATIC TRUE)
ENDIF(NOT DEFINED PODOFO_BUILD_STATIC)

IF(WIN32)
    # On win32 we support EITHER shared OR static builds.
    # If both are enabled (default), turn off generation of the
    # static library.
    IF(PODOFO_BUILD_SHARED AND PODOFO_BUILD_STATIC)
         MESSAGE("Win32: Both PODOFO_BUILD_SHARED and PODOFO_BUILD_STATIC set")
         MESSAGE("Win32: Disabling static library generation.")
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

FIND_PATH(LIBZ_H NAMES zlib.h)
FIND_LIBRARY(LIBZ_LIB NAMES z zlib libz)
MESSAGE("Found zlib headers in ${LIBZ_H}, library at ${LIBZ_LIB}")

FIND_PATH(LIBJPEG_H NAMES jpeglib.h)
FIND_LIBRARY(LIBJPEG_LIB NAMES jpeg libjpeg jpeglib)
MESSAGE("Found libjpeg headers in ${LIBJPEG_H}, library at ${LIBJPEG_LIB}")

# Freetype puts its headers in stupid places and expects you to run a
# non-portable shell script to find them.  Find ft2build.h and try to determine
# where freetype.h is relative to it.
# TODO: use pkg-config and/or freetype-config to find freetype when possible
FIND_PATH(LIBFREETYPE_FT2BUILD_H NAMES ft2build.h)
MESSAGE("Found ft2build.h in ${LIBFREETYPE_FT2BUILD_H}")
# Now try to find the corresponding freetype.h
FIND_PATH(LIBFREETYPE_FREETYPE_H
    NAMES freetype/freetype.h
    PATHS
    ${LIBFREETYPE_FT2BUILD_H}/freetype2
    ${LIBFREETYPE_FT2BUILD_H}/freetype
    )
MESSAGE("Found freetypt.h in ${LIBFREETYPE_FREETYPE_H}")
# At least the library will be somewhere sensible
FIND_LIBRARY(LIBFREETYPE_LIB NAMES freetype libfreetype)
MESSAGE("Found freetype library at ${LIBFREETYPE_LIB}")

IF(WANT_FONTCONFIG)
    # fontconfig often lives in a subdir off the include path
    # called just `fontconfig'
    FIND_PATH(LIBFONTCONFIG_INCLUDEPATH NAMES fontconfig.h)
    IF(LIBFONTCONFIG_INCLUDEPATH)
        MESSAGE("Found fontconfig.h at ${LIBFONTCONFIG_H}/fontconfig.h}")
        SET(LIBFONTCONFIG_H ${LIBFONTCONFIG_INCLUDEPATH})
    ELSE(LIBFONTCONFIG_INCLUDEPATH)
        FIND_PATH(LIBFONTCONFIG_INCLUDEPATH_PARENTDIR NAMES fontconfig/fontconfig.h)
        MESSAGE("Found fontconfig.h at ${LIBFONTCONFIG_INCLUDEPATH_PARENTDIR}/fontconfig/fontconfig.h")
        SET(LIBFONTCONFIG_H ${LIBFONTCONFIG_INCLUDEPATH_PARENTDIR}/fontconfig)
    ENDIF(LIBFONTCONFIG_INCLUDEPATH)
    # The fontconfig library is generally easy to find
    FIND_LIBRARY(LIBFONTCONFIG_LIB NAMES fontconfig)
    MESSAGE("Found fontconfig library at ${LIBFONTCONFIG_LIB}")
    SET(PODOFO_LIB_FONTCONFIG:STRING fontconfig)
    ADD_DEFINITIONS(-DHAVE_FONTCONFIG)
ELSE(WANT_FONTCONFIG)
    SET(PODOFO_LIB_FONTCONFIG:STRING)
    SET(LIBFONTCONFIG_H:PATH)
    SET(LIBFONTCONFIG_LIB:PATH)
ENDIF(WANT_FONTCONFIG)

INCLUDE_DIRECTORIES(
    src
    ${LIBFREETYPE_FT2BUILD_H}
    ${LIBFREETYPE_FREETYPE_H}
    ${LIBZ_H}
    ${LIBJPEG_H}
    ${LIBFONTCONFIG_H}
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
    ${LIBFONTCONFIG_LIB}
    ${PLATFORM_SYSTEM_LIBRARIES}
    )

SET(PODOFO_LIB
    podofo
    ${PODOFO_LIB_DEPENDS}
    )

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(tools)
