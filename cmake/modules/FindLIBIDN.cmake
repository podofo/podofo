# - Find Libidn
# Find the native Libidn includes and library
#
#  Libidn_INCLUDE_DIR - where to find stringprep.h, etc.
#  Libidn_LIBRARIES   - List of libraries when using libidn.
#  Libidn_FOUND       - True if libidn found.

if (Libidn_INCLUDE_DIR)
  # Already in cache, be silent
  set(Libidn_FIND_QUIETLY TRUE)
endif ()

find_path(Libidn_INCLUDE_DIR stringprep.h)

set(Libidn_LIBRARY_NAMES_RELEASE ${Libidn_LIBRARY_NAMES_RELEASE} ${Libidn_LIBRARY_NAMES} idn libidn)
find_library(Libidn_LIBRARY_RELEASE NAMES ${Libidn_LIBRARY_NAMES_RELEASE})

# Find a debug library if one exists and use that for debug builds.
# This really only does anything for win32, but does no harm on other
# platforms.
set(Libidn_LIBRARY_NAMES_DEBUG ${Libidn_LIBRARY_NAMES_DEBUG} idnd libidnd)
find_library(Libidn_LIBRARY_DEBUG NAMES ${Libidn_LIBRARY_NAMES_DEBUG})

include(LibraryDebugAndRelease)
set_library_from_debug_and_release(Libidn)

# handle the QUIETLY and REQUIRED arguments and set Libidn_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libidn DEFAULT_MSG Libidn_LIBRARY Libidn_INCLUDE_DIR)

if(Libidn_FOUND)
  set(Libidn_LIBRARIES ${Libidn_LIBRARY})
else()
  set(Libidn_LIBRARIES)
endif()

mark_as_advanced(Libidn_LIBRARY Libidn_INCLUDE_DIR)
