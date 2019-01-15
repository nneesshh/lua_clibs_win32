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

#ifndef LUA_EXTERN
# if defined(WIN32) || defined(_WIN32)
/*  Building shared library. */
#   define LUA_EXTERN __declspec(dllexport)
# elif __GNUC__ >= 4
#  define LUA_EXTERN __attribute__((visibility("default")))
# else
#  define LUA_EXTERN /* nothing */
# endif
#endif

LUA_EXTERN int luaopen_int64(lua_State *L);

#ifdef __cplusplus
}
#endif