#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

struct sprite_pack {
	int n;
	int cap;
	struct stbrp_rect rect[1];
};

static int
lspritepack_add(lua_State *L) {
	struct sprite_pack *sp = lua_touserdata(L, 1);
	if (sp == NULL)
		return luaL_error(L, "Invalid spritepack_add call");
	int index = sp->n;
	if (index >= sp->cap)
		return luaL_error(L, "Too many sprite %d", index);
	int id = luaL_checkinteger(L, 2);
	int w = luaL_checkinteger(L, 3);
	int h = luaL_checkinteger(L, 4);
	++sp->n;
	struct stbrp_rect *r = &sp->rect[index];
	r->id = id;
	r->w = w;
	r->h = h;
	return 0;
}

#define MAX_NODE 8192

static int
lspritepack_run(lua_State *L) {
	struct sprite_pack *sp = lua_touserdata(L, 1);
	if (sp == NULL)
		return luaL_error(L, "Invalid spritepack_pack call");
	int width = luaL_checkinteger(L, 2);
	int height = luaL_optinteger(L, 3, width);
	
	stbrp_context ctx;
	stbrp_node tmp[MAX_NODE];

	stbrp_init_target (&ctx, width, height, tmp, MAX_NODE);
	int succ = stbrp_pack_rects (&ctx, sp->rect, sp->n);
	int n = sp->n;
	sp->n = 0;
	if (succ) {
		lua_newtable(L);
		int i;
		for (i=0;i<n;i++) {
			struct stbrp_rect *r = &sp->rect[i];
			uint64_t v = (uint64_t)r->x << 32 | r->y;
			lua_pushinteger(L, v);
			lua_rawseti(L, -2, r->id);
		}
		return 1;
	} else {
		lua_newtable(L);	// result
		lua_newtable(L);	// can't pack
		int i;
		int index = 0;
		for (i=0;i<n;i++) {
			struct stbrp_rect *r = &sp->rect[i];
			if (r->was_packed) {
				uint64_t v = (uint64_t)r->x << 32 | r->y;
				lua_pushinteger(L, v);
				lua_rawseti(L, -3, r->id);
			} else {
				lua_pushinteger(L, r->id);
				lua_rawseti(L, -2, ++index);
			}
		}
		return 2;
	}
}

static int
lspritepack_pack(lua_State *L) {
	int cap = luaL_checkinteger(L, 1);
	struct sprite_pack *sp = (struct sprite_pack *)lua_newuserdatauv(L, sizeof(*sp) + (cap - 1) * sizeof(sp->rect[0]), 0);
	sp->n = 0;
	sp->cap = cap;

	if (luaL_newmetatable(L, "SOLUNA_SPRITEPACK")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "add", lspritepack_add },
			{ "run", lspritepack_run },
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
luaopen_spritepack(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "pack", lspritepack_pack },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
