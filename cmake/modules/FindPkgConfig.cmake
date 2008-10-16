## FindPkgConfig.cmake
## by Albert Strasheim <http://students . ee . sun . ac . za/~albert/>
##
## This module finds packages using pkg-config, which retrieves
## information about packages from special metadata files.
##
## See http://www . freedesktop . org/Software/pkgconfig/
##
## -------------------------------------------------------------------
##
## Usage:
##
## INCLUDE( ${CMAKE_ROOT}/Modules/FindPkgConfig.cmake)
## PKGCONFIG("libxml-2.0 >= 1.3")
## IF(PKGCONFIG_FOUND)
##   # do something with CMAKE_PKGCONFIG_C_FLAGS
##   # do something with PKGCONFIG_LIBRARIES
## ELSE(PKGCONFIG_FOUND)
#    MESSAGE("Cannot find libxml2 version 1.3 or above")
## ENDIF(PKGCONFIG_FOUND)
##
## Notes:
## 
## You can set the PKG_CONFIG_PATH environment variable to tell
## pkg-config where to search for .pc files. See pkg-config(1) for
## more information.

#FIXME: IF(WIN32) pkg-config --msvc-syntax ENDIF(WIN32) ???

FIND_PROGRAM(CMAKE_PKGCONFIG_EXECUTABLE pkg-config)
MARK_AS_ADVANCED(CMAKE_PKGCONFIG_EXECUTABLE)

MACRO(PKGCONFIG LIBRARY)
  SET(PKGCONFIG_FOUND 0)

#  MESSAGE("DEBUG: find library '${LIBRARY}'")
  
  IF(CMAKE_PKGCONFIG_EXECUTABLE)
#    MESSAGE("DEBUG: pkg-config executable found")
    
    EXEC_PROGRAM(${CMAKE_PKGCONFIG_EXECUTABLE}
      ARGS "'${LIBRARY}'"
      OUTPUT_VARIABLE PKGCONFIG_OUTPUT
      RETURN_VALUE PKGCONFIG_RETURN)
    IF(NOT PKGCONFIG_RETURN)
#      MESSAGE("DEBUG: packages found")
      
      # set C_FLAGS and CXX_FLAGS
      EXEC_PROGRAM(${CMAKE_PKGCONFIG_EXECUTABLE}
        ARGS "--cflags '${LIBRARY}'"
        OUTPUT_VARIABLE CMAKE_PKGCONFIG_C_FLAGS)
      SET(CMAKE_PKGCONFIG_CXX_FLAGS "${CMAKE_PKGCONFIG_C_FLAGS}")
      
      # set LIBRARIES
      EXEC_PROGRAM(${CMAKE_PKGCONFIG_EXECUTABLE}
        ARGS "--libs '${LIBRARY}'"
        OUTPUT_VARIABLE PKGCONFIG_LIBRARIES)
      
      SET(PKGCONFIG_FOUND 1)
    ELSE(NOT PKGCONFIG_RETURN)
#      MESSAGE("DEBUG '${LIBRARY}' NOT FOUND by pkg-config")
      
      SET(CMAKE_PKGCONFIG_C_FLAGS "")
      SET(CMAKE_PKGCONFIG_CXX_FLAGS "")
      SET(PKGCONFIG_LIBRARIES "")
    ENDIF(NOT PKGCONFIG_RETURN)
#  ELSE(CMAKE_PKGCONFIG_EXECUTABLE)
#    MESSAGE("DEBUG: pkg-config executable NOT FOUND")
  ENDIF(CMAKE_PKGCONFIG_EXECUTABLE)

#  MESSAGE("DEBUG: CMAKE_PKGCONFIG_C_FLAGS=${CMAKE_PKGCONFIG_C_FLAGS}")
#  MESSAGE("DEBUG: PKGCONFIG_LIBRARIES=${PKGCONFIG_LIBRARIES}")
ENDMACRO(PKGCONFIG)

