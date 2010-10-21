cmake_minimum_required(VERSION 2.6)
IF(NOT PODOFO_MAIN_CMAKELISTS_READ)
  MESSAGE(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
ENDIF(NOT PODOFO_MAIN_CMAKELISTS_READ)

SET(PODOFO_BASE_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")
SET(PODOFO_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")

ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(doc)

SET(PODOFO_HEADERS
  podofo-base.h
  podofo.h
  )


INSTALL(FILES ${PODOFO_HEADERS}
  DESTINATION "include/podofo"
  )

