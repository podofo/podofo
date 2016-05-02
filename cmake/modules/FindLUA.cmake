# LUA_FOUND - system has LUA
# LUA_LIBRARIES - Link these to use LUA
# LUA_DEFINITIONS - Compiler switches required for using LUA
#
# Based on FindFONTCONFIG.cmake Copyright (c) 2006,2007 Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# TODO: Update this code to handle debug/release builds on win32.

if (LUA_LIBRARIES AND LUA_INCLUDE_DIR)

  # in cache already
  set(LUA_FOUND TRUE)

else (LUA_LIBRARIES AND LUA_INCLUDE_DIR)

  FIND_PACKAGE(Lua51)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lua DEFAULT_MSG LUA_LIBRARIES LUA_INCLUDE_DIR )
  
  mark_as_advanced(LUA_LIBRARIES LUA_INCLUDE_DIR)

endif (LUA_LIBRARIES AND LUA_INCLUDE_DIR)
