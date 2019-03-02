#include <lua.h>
#include <lauxlib.h>
#include <assert.h>

#include "memory_stream.h"

static const struct luaL_Reg _lua_c_utility[] = {
	{ NULL, NULL }
};

LUALIB_API int
luaopen_lcutil(lua_State *L)
{
	int top = lua_gettop(L);
	
#if LUA_VERSION_NUM < 502
	luaL_register(L, "lcutil", _lua_c_utility);
#else
	luaL_newlib(L, _lua_c_utility);
#endif

	assert(1 == lua_gettop(L) - top);
	return 1;
}