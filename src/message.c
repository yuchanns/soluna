#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include "message.h"

static int
lmessage_unpack(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	struct soluna_message *m = (struct soluna_message *)lua_touserdata(L,1);
	lua_pushstring(L, m->type);
	lua_pushinteger(L, m->v.p[0]);
	lua_pushinteger(L, m->v.p[1]);
	lua_pushinteger(L, m->v.u64);
	message_release(m);
	return 4;
}

int
luaopen_appmessage(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "unpack", lmessage_unpack },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}