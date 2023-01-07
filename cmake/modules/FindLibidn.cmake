# - Find Libidn
# Find the native Libidn includes and library
#
#  LIBIDN_INCLUDE_DIR - where to find stringprep.h, etc.
#  LIBIDN_LIBRARIES   - List of libraries when using libidn.
#  LIBIDN_FOUND       - True if libidn found.

if (LIBIDN_INCLUDE_DIR)
  # Already in cache, be silent
  set(LIBIDN_FIND_QUIETLY TRUE)
endif ()

find_path(LIBIDN_INCLUDE_DIR stringprep.h)

set(LIBIDN_LIBRARY_NAMES_RELEASE ${LIBIDN_LIBRARY_NAMES_RELEASE} ${LIBIDN_LIBRARY_NAMES} idn libidn)
find_library(LIBIDN_LIBRARY_RELEASE NAMES ${LIBIDN_LIBRARY_NAMES_RELEASE})

# Find a debug library if one exists and use that for debug builds.
# This really only does anything for win32, but does no harm on other
# platforms.
set(LIBIDN_LIBRARY_NAMES_DEBUG ${LIBIDN_LIBRARY_NAMES_DEBUG} idnd libidnd)
find_library(LIBIDN_LIBRARY_DEBUG NAMES ${LIBIDN_LIBRARY_NAMES_DEBUG})

include(LibraryDebugAndRelease)
set_library_from_debug_and_release(LIBIDN)

# handle the QUIETLY and REQUIRED arguments and set LIBIDN_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libidn DEFAULT_MSG LIBIDN_LIBRARY LIBIDN_INCLUDE_DIR)

if(LIBIDN_FOUND)
  set(LIBIDN_LIBRARIES ${LIBIDN_LIBRARY})
else()
  set(LIBIDN_LIBRARIES)
endif()

mark_as_advanced(LIBIDN_LIBRARY LIBIDN_INCLUDE_DIR)
