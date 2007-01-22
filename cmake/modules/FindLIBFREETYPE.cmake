# Freetype puts its headers in stupid places and expects you to run a
# non-portable shell script to find them.  Find ft2build.h and try to determine
# where freetype.h is relative to it.
# TODO: use pkg-config and/or freetype-config to find freetype when possible
FIND_PATH(LIBFREETYPE_FT2BUILD_H NAMES ft2build.h)
MESSAGE("Found ft2build.h in ${LIBFREETYPE_FT2BUILD_H}")
# Now try to find the corresponding freetype.h
# TODO: avoid finding /usr/include/freetype/freetype.h from freetype1
FIND_PATH(LIBFREETYPE_FREETYPE_H
    NAMES freetype/freetype.h
    PATHS
    ${LIBFREETYPE_FT2BUILD_H}/freetype2
    ${LIBFREETYPE_FT2BUILD_H}/freetype
    )
MESSAGE("Found freetype.h in ${LIBFREETYPE_FREETYPE_H}")
# At least the library will be somewhere sensible
FIND_LIBRARY(LIBFREETYPE_LIB NAMES freetype libfreetype)
MESSAGE("Found freetype library at ${LIBFREETYPE_LIB}")

IF(LIBFREETYPE_FT2BUILD_H AND LIBFREETYPE_FREETYPE_H AND LIBFREETYPE_LIB)
	SET(LIBFREETYPE_FOUND TRUE CACHE BOOLEAN "Was libfreetype found")
ELSE(LIBFREETYPE_FT2BUILD_H AND LIBFREETYPE_FREETYPE_H AND LIBFREETYPE_LIB)
	SET(LIBFREETYPE_FOUND FALSE CACHE BOOLEAN "Was libfreetype found")
ENDIF(LIBFREETYPE_FT2BUILD_H AND LIBFREETYPE_FREETYPE_H AND LIBFREETYPE_LIB)
