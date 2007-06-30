# fontconfig lives in a subdir off the include path called just `fontconfig'
FIND_PATH(LIBFONTCONFIG_H NAMES fontconfig/fontconfig.h)
MESSAGE("Found fontconfig.h at ${LIBFONTCONFIG_H}/fontconfig/fontconfig.h")

FIND_LIBRARY(LIBFONTCONFIG_LIB NAMES fontconfig)
MESSAGE("Found fontconfig library at ${LIBFONTCONFIG_LIB}")

IF(LIBFONTCONFIG_H AND LIBFONTCONFIG_LIB)
        SET(LIBFONTCONFIG_FOUND TRUE CACHE BOOLEAN "Was fontconfig found")
ELSE(LIBFONTCONFIG_H AND LIBFONTCONFIG_LIB)
        SET(LIBFONTCONFIG_FOUND FALSE CACHE BOOLEAN "Was fontconfig found")
ENDIF(LIBFONTCONFIG_H AND LIBFONTCONFIG_LIB)
