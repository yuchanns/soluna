#include <lua.h>
#include <lauxlib.h>
#include "ltask/src/cond.h"

struct event_context {
	struct cond ev;
};

static int
levent_wait(lua_State *L) {
	struct event_context *ctx = lua_touserdata(L, 1);
	cond_wait_begin(&ctx->ev);
	cond_wait(&ctx->ev);
	cond_wait_end(&ctx->ev);
	return 0;
}

static int
levent_trigger(lua_State *L) {
	struct event_context *ctx = lua_touserdata(L, 1);
	cond_trigger_begin(&ctx->ev);
	cond_trigger_end(&ctx->ev, 1);
	return 0;
}

static int
levent_ptr(lua_State *L) {
	void * ev = luaL_checkudata(L, 1, "SOLUNA_EVENT");
	lua_pushlightuserdata(L, ev);
	return 1;
}

static int
levent_release(lua_State *L) {
	struct event_context *ctx = lua_touserdata(L, 1);
	cond_release(&ctx->ev);
	return 0;
}

static int
levent_create(lua_State *L) {
	struct event_context *ctx = (struct event_context *)lua_newuserdatauv(L, sizeof(*ctx), 0);
	cond_create(&ctx->ev);
	if (luaL_newmetatable(L, "SOLUNA_EVENT")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "__gc", levent_release },
			{ "ptr", levent_ptr },
			{ "wait", levent_wait },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int
luaopen_soluna_event(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "create", levent_create },
		{ "trigger", levent_trigger },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
