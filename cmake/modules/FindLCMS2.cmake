# - Find LCMS2 library
# Find the native LCMS2 includes and library
# Once done this will define
#
#  LCMS2_INCLUDE_DIR    - Where to find lcms2.h, etc.
#  LCMS2_LIBRARIES      - Libraries to link against to use LCMS2.
#  LCMS2_FOUND          - If false, do not try to use LCMS2.
#
# also defined, but not for general use are
#  LCMS2_LIBRARY        - Where to find the LCMS2 library.

if (LCMS2_INCLUDE_DIR)
  # Already in cache, be silent
  set(LCMS2_FIND_QUIETLY TRUE)
endif ()

find_path(LCMS2_INCLUDE_DIR lcms2.h)

set(LCMS2_LIBRARY_NAMES_RELEASE ${LCMS2_LIBRARY_NAMES_RELEASE} ${LCMS2_LIBRARY_NAMES} lcms2 liblcms2)
find_library(LCMS2_LIBRARY_RELEASE NAMES ${LCMS2_LIBRARY_NAMES_RELEASE})

# Find a debug library if one exists and use that for debug builds.
# This really only does anything for win32, but does no harm on other
# platforms.
set(LCMS2_LIBRARY_NAMES_DEBUG ${LCMS2_LIBRARY_NAMES_DEBUG} lcms2d liblcms2d)
find_library(LCMS2_LIBRARY_DEBUG NAMES ${LCMS2_LIBRARY_NAMES_DEBUG})

include(LibraryDebugAndRelease)
set_library_from_debug_and_release(LCMS2)

# handle the QUIETLY and REQUIRED arguments and set LCMS2_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LCMS2 DEFAULT_MSG LCMS2_LIBRARY LCMS2_INCLUDE_DIR)

if(LCMS2_FOUND)
  set(LCMS2_LIBRARIES ${LCMS2_LIBRARY})
else()
  set(LCMS2_LIBRARIES)
endif()

mark_as_advanced(LCMS2_LIBRARY LCMS2_INCLUDE_DIR)
