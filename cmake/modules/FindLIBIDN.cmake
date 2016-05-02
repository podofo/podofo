# - Find libidn
# Find the native LIBIDN includes and library
#
#  LIBIDN_INCLUDE_DIR - where to find stringprep.h, etc.
#  LIBIDN_LIBRARIES   - List of libraries when using libidn.
#  LIBIDN_FOUND       - True if libidn found.


IF (LIBIDN_INCLUDE_DIR)
  # Already in cache, be silent
  SET(LIBIDN_FIND_QUIETLY TRUE)
ENDIF (LIBIDN_INCLUDE_DIR)

FIND_PATH(LIBIDN_INCLUDE_DIR stringprep.h)

SET(LIBIDN_LIBRARY_NAMES_RELEASE ${LIBIDN_LIBRARY_NAMES_RELEASE} ${LIBIDN_LIBRARY_NAMES} idn)
FIND_LIBRARY(LIBIDN_LIBRARY_RELEASE NAMES ${LIBIDN_LIBRARY_NAMES_RELEASE} )

# Find a debug library if one exists and use that for debug builds.
# This really only does anything for win32, but does no harm on other
# platforms.
SET(LIBIDN_LIBRARY_NAMES_DEBUG ${LIBIDN_LIBRARY_NAMES_DEBUG} idnd)
FIND_LIBRARY(LIBIDN_LIBRARY_DEBUG NAMES ${LIBIDN_LIBRARY_NAMES_DEBUG})

INCLUDE(LibraryDebugAndRelease)
SET_LIBRARY_FROM_DEBUG_AND_RELEASE(LIBIDN)

# handle the QUIETLY and REQUIRED arguments and set LIBIDN_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBIDN DEFAULT_MSG LIBIDN_LIBRARY LIBIDN_INCLUDE_DIR)

IF(LIBIDN_FOUND)
  SET( LIBIDN_LIBRARIES ${LIBIDN_LIBRARY} )
ELSE(LIBIDN_FOUND)
  SET( LIBIDN_LIBRARIES )
ENDIF(LIBIDN_FOUND)

MARK_AS_ADVANCED( LIBIDN_LIBRARY LIBIDN_INCLUDE_DIR )

