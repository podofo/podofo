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
    pkg_check_modules(LUA lua5.1)
  endif (PKG_CONFIG_FOUND)

  find_path(LUA_INCLUDE_DIR lua.h
    PATHS
    ${LUA_INCLUDE_DIRS}
  )

  find_library(LUA_LIBRARIES NAMES liblua5.1
    PATHS
    ${LUA_LIBRARY_DIRS}
  )

  include(PoDoFoFindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lua DEFAULT_MSG LUA_LIBRARIES LUA_INCLUDE_DIR )
  
  mark_as_advanced(LUA_LIBRARIES LUA_INCLUDE_DIR)

endif (LUA_LIBRARIES AND LUA_INCLUDE_DIR)
