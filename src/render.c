#include <lua.h>
#include <lauxlib.h>
#include <string.h>

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_app.h"

// todo : offscreen pass
struct pass {
	sg_pass_action pass_action;
};

static int
read_color_action(lua_State *L, int index, sg_pass_action *action, int idx) {
	char key[] = { 'c', 'o', 'l', 'o' , 'r' , '0' + idx, '\0' };
	int t = lua_getfield(L, index, key);
	if (t == LUA_TNIL) {
		lua_pop(L, 1);
		return 0;
	}
	if (idx >= SG_MAX_COLOR_ATTACHMENTS)
		return luaL_error(L, "Too many color attachments %d >= %d", idx , SG_MAX_COLOR_ATTACHMENTS);
	if ( t == LUA_TSTRING ) {
		const char * key = lua_tostring(L, -1);
		if (strcmp(key, "load") == 0) {
			action->colors[idx].load_action = SG_LOADACTION_LOAD;
		} else if (strcmp(key, "dontcare") == 0 ) {
			action->colors[idx].load_action = SG_LOADACTION_DONTCARE;
		} else {
			return luaL_error(L, "Invalid load action (%d) = %s", idx, key);
		}
	} else {
		uint32_t c = luaL_checkinteger(L, -1);
		if (c <= 0xffffff) {
			action->colors[idx].clear_value.a = 1.0f;
		} else {
			action->colors[idx].clear_value.a = ((c & 0xff000000) >> 24) / 255.0f;
		}
		action->colors[idx].clear_value.r = ((c & 0xff0000) >> 16) / 255.0f;
		action->colors[idx].clear_value.g = ((c & 0x00ff00) >> 8) / 255.0f;
		action->colors[idx].clear_value.b = ((c & 0x0000ff)) / 255.0f;
		action->colors[idx].load_action = SG_LOADACTION_CLEAR;
	}
	lua_pop(L, 1);
	return 1;
}

static int
lpass_begin(lua_State *L) {
	struct pass * p = (struct pass *)luaL_checkudata(L, 1, "SOKOL_PASS");
	sg_begin_pass(&(sg_pass) { .action = p->pass_action, .swapchain = sglue_swapchain() });
	return 0;
}

static int
lpass_end(lua_State *L) {
	sg_end_pass();
	return 0;
}

static int
lpass_new(lua_State *L) {
	struct pass * p = lua_newuserdatauv(L, sizeof(*p), 0);
	memset(p, 0, sizeof(*p));
	luaL_checktype(L, 1, LUA_TTABLE);
	sg_pass_action *action = &p->pass_action;
	// todo : store action
	
	int i = 0;
	while (read_color_action(L, 1, action, i)) {
		++i;
	}
	if (lua_getfield(L, 1, "depth") == LUA_TNIL) {
		action->depth.load_action = SG_LOADACTION_DONTCARE;
	} else {
		float depth = luaL_checknumber(L, -1);
		action->depth.load_action = SG_LOADACTION_CLEAR;
		action->depth.clear_value = depth;
	}
	lua_pop(L, 1);
	if (lua_getfield(L, 1, "stencil") == LUA_TNIL) {
		action->depth.load_action = SG_LOADACTION_DONTCARE;
	} else {
		int s = luaL_checkinteger(L, -1);
		if (s < 0 || s > 255)
			return luaL_error(L, "Invalid stencil %d", s);
		action->depth.load_action = SG_LOADACTION_CLEAR;
		action->depth.clear_value = s;
	}
	lua_pop(L, 1);
	if (luaL_newmetatable(L, "SOKOL_PASS")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "begin", lpass_begin },
			{ "finish", lpass_end },	// end is a reserved keyword in lua, use finish instead
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);
		
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

static int
lsubmit(lua_State *L) {
	sg_commit();
	return 0;
}

int
luaopen_render(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "pass", lpass_new },
		{ "submit", lsubmit },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}