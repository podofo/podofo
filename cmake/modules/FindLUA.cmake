# LUA_FOUND - system has Fontconfig
# LUA_LIBRARIES - Link these to use LUA
# LUA_DEFINITIONS - Compiler switches required for using LUA
#
# Based on FindFONTCONFIG.cmake Copyright (c) 2006,2007 Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# TODO: Update this code to handle debug/release builds on win32.

FIND_PACKAGE(PkgConfig)

if (LUA_LIBRARIES AND LUA_INCLUDE_DIR)

  # in cache already
  set(LUA_FOUND TRUE)

else (LUA_LIBRARIES AND LUA_INCLUDE_DIR)

  if (PKG_CONFIG_FOUND)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    pkg_check_modules(LUA51 lua5.1)

    # If we can't find Lua 5.1, try for Lua 5.0
    # TODO: set compile flag for lua version
    IF(NOT LUA51_FOUND)
        pkg_check_modules(LUA50 lua50)
        pkg_check_modules(LUALIB50 lualib50)
    ENDIF(NOT LUA51_FOUND)

  endif (PKG_CONFIG_FOUND)


  find_library(LUA_LIBRARIES NAMES lua5.1
    PATHS
    ${LUA51_LIBRARY_DIRS}
  )

  IF(LUA_LIBRARIES)
    # We found the lua 5.1 library, so locate the headers for it.
    find_path(LUA_INCLUDE_DIR lua.h
      PATHS
      ${LUA51_INCLUDE_DIRS}
    )
  ELSE(LUA_LIBRARIES)
    # Lua 5.1 library not found. Look for Lua 5.0 .
    find_library(LUA_LIBRARIES NAMES lua50
      ${LUA50_LIBRARY_DIRS}
      PATHS
      ${LUA51_LIBRARY_DIRS}
    )
    IF(LUA_LIBRARIES)
      # Lua 5.0 found. Make sure we can find the extension library we require as well.
      find_library(LUALIB_LIBRARIES NAMES lualib50
        PATHS
        ${LUALIB50_LIBRARY_DIRS}
        ${LUA50_LIBRARY_DIRS}
      )
      IF(NOT LUALIB_LIBRARIES)
        # Lua 5.0 is useless without extension libs, so clear the
        # Lua libraries variable.
        SET(LUA_LIBRARIES)
        MESSAGE("-- Lua 5.0 libraries found, but no lualib extension lib. Cannot use lua 5.0.")
      ELSE(NOT LUALIB_LIBRARIES)
        SET(LUA_LIBRARIES ${LUA_LIBRARIES} ${LUALIB_LIBRARIES})
        # Now find the headers we need
        find_path(LUA_INCLUDE_DIR lua.h
          PATHS
          ${LUA50_INCLUDE_DIRS}
        )
        find_path(LUALIB_INCLUDE_DIR lauxlib.h
          PATHS
          ${LUALIB50_INCLUDE_DIRS}
          ${LUA50_INCLUDE_DIRS}
        )
        IF(LUA_INCLUDE_DIR AND LUALIB_INCLUDE_DIR)
          # We found both headers, so we're good to go.
          IF(NOT LUALIB_INCLUDE_DIR STREQUAL LUA_INCLUDE_DIR)
            SET(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIR} ${LUALIB_INCLUDE_DIR})
          ENDIF(NOT LUALIB_INCLUDE_DIR STREQUAL LUA_INCLUDE_DIR)
        ELSE(LUA_INCLUDE_DIR AND LUALIB_INCLUDE_DIR)
          # lua.h or lauxlib.h was missing
          MESSAGE("-- lua 5.0 library found but lua.h or lauxlib.h not found, cannot use Lua 5.0")
          SET(LUA_LIBRARIES)
          SET(LUA_INCLUDE_DIR)
        ENDIF(LUA_INCLUDE_DIR AND LUALIB_INCLUDE_DIR)
      ENDIF(NOT LUALIB_LIBRARIES)
    ENDIF(LUA_LIBRARIES)
  ENDIF(LUA_LIBRARIES)

  include(PoDoFoFindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lua DEFAULT_MSG LUA_LIBRARIES LUA_INCLUDE_DIR )
  
  mark_as_advanced(LUA_LIBRARIES LUA_INCLUDE_DIR)

endif (LUA_LIBRARIES AND LUA_INCLUDE_DIR)
