#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <stdint.h>
#include <stdbool.h>

#if LUA_VERSION_NUM < 502
bool lbind_int64_isint64(lua_State* L, int idx);
void lbind_int64_pushint64(lua_State *L, int64_t n);
int64_t lbind_int64_toint64(lua_State *L, int idx);
#endif

LUALIB_API int luaopen_int64(lua_State *L);

#ifdef __cplusplus
}
#endif