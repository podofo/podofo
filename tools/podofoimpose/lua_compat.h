extern "C" {
#include "lua.h"
// Note: If you're missing these, you're using lua 5.0 and haven't installed
// the extension libraries.
#include "lualib.h"
#include "lauxlib.h"
}

#if !defined(LUA_VERSION_NUM)
// Old lua without numeric version
#define LUA_VERSION_NUM 0
#endif

// Handle an API difference in the lua_open call between
// Lua 5.1 and Lua 5.2.
#if LUA_VERSION_NUM >= 502
inline lua_State* imp_lua_open(void) {
    return luaL_newstate();
}
#else
inline lua_State* imp_lua_open(void) {
    return lua_open();
}
#endif

// Handle an API difference in the dofile and getn calls between
// Lua 5.0 and Lua 5.1.
#if LUA_VERSION_NUM >= 501
inline int imp_lua_dofile(lua_State* L, const char * path) {
    return luaL_dofile(L, path);
}
#else
inline int imp_lua_dofile(lua_State* L, const char * path) {
    return lua_dofile(L, path);
}
#endif
