# - Find TIFF library
# Find the native TIFF includes and library
# This module defines
#  TIFF_INCLUDE_DIR, where to find tiff.h, etc.
#  TIFF_LIBRARIES, libraries to link against to use TIFF.
#  TIFF_FOUND, If false, do NOT try to use TIFF.
# also defined, but NOT for general use are
#  TIFF_LIBRARY, where to find the TIFF library.
#
# Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
# See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

if (TIFF_INCLUDE_DIR)
  # Already in cache, be silent
  set(TIFF_FIND_QUIETLY TRUE)
endif (TIFF_INCLUDE_DIR)

find_path(TIFF_INCLUDE_DIR NAMES tiff.h )

set(TIFF_NAMES ${TIFF_NAMES} tiff libtiff)
find_library(TIFF_LIBRARY NAMES ${TIFF_NAMES} )

if (TIFF_INCLUDE_DIR AND TIFF_LIBRARY)
   set(TIFF_FOUND TRUE)
   set(TIFF_LIBRARIES ${TIFF_LIBRARY} )
else (TIFF_INCLUDE_DIR AND TIFF_LIBRARY)
   set(TIFF_FOUND FALSE)
endif (TIFF_INCLUDE_DIR AND TIFF_LIBRARY)

if (TIFF_FOUND)
   if (NOT TIFF_FIND_QUIETLY)
      message(STATUS "Found TIFF: ${TIFF_LIBRARY}")
   endif (NOT TIFF_FIND_QUIETLY)
else (TIFF_FOUND)
   if (TIFF_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find TIFF")
   endif (TIFF_FIND_REQUIRED)
endif (TIFF_FOUND)

mark_as_advanced(TIFF_INCLUDE_DIR TIFF_LIBRARY)
