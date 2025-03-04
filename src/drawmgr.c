#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>

#include "batch.h"
#include "spritemgr.h"

struct draw_element {
	struct draw_primitive * base;
	int n;
	int material;
};

struct drawmgr {
	struct sprite_bank *bank;
	int cap;
	int n;
	int bank_n;
	struct draw_element data[1];
};

static int
ldrawmgr_len(lua_State *L) {
	struct drawmgr * d = lua_touserdata(L, 1);
	lua_pushinteger(L, d->n);
	return 1;
}

static int
ldrawmgr_index(lua_State *L) {
	struct drawmgr * d = lua_touserdata(L, 1);
	int idx = luaL_checkinteger(L, 2) - 1;
	if (idx < 0 || idx >= d->n) {
		return 0;
	}
	struct draw_element *e = &(d->data[idx]);
	if (e->material < 0) {
		lua_pushinteger(L, -e->material);
	} else {
		lua_pushinteger(L, 0);
	}
	lua_pushlightuserdata(L, e->base);
	lua_pushinteger(L, e->n);
	if (e->material >= 0) {
		lua_pushinteger(L, e->material);
		return 4;
	} else {
		return 3;
	}
}

static int
ldrawmgr_reset(lua_State *L) {
	struct drawmgr * d = (struct drawmgr *)luaL_checkudata(L, 1, "SOLUNA_DRAWMGR");
	d->n = 0;
	d->bank_n = d->bank->n;
	return 0;
}

static int
append_external_material(struct drawmgr * d, struct draw_primitive *base, int n, int matid) {
	int i;
	for (i=1;i<n;i++) {
		if (base[i*2].sprite != matid) {
			break;
		}
	}
	struct draw_element *e = &d->data[d->n++];
	e->base = base;
	e->n = n;
	e->material = matid;
	return n;
}

static int
append_default_material(struct drawmgr * d, struct draw_primitive *base, int n, int texid) {
	int i;
	struct sprite_rect * rect = d->bank->rect;
	int rect_n = d->bank_n;

	for (i=1;i<n;i++) {
		int sprite = base[i].sprite;
		if (sprite <= 0 || sprite > rect_n)
			break;
		--sprite;
		if (texid != rect[sprite].texid)
			break;
	}
	struct draw_element *e = &d->data[d->n++];
	e->base = base;
	e->n = n;
	e->material = texid;
	return n;
}

static int
ldrawmgr_append(lua_State *L) {
	struct drawmgr * d = (struct drawmgr *)luaL_checkudata(L, 1, "SOLUNA_DRAWMGR");
	
	struct draw_primitive *prim = (struct draw_primitive *)lua_touserdata(L, 2);
	int prim_n = luaL_checkinteger(L, 3);
	
	struct sprite_rect * rect = d->bank->rect;
	int rect_n = d->bank_n;

	int i;
	struct draw_primitive *end_ptr = &prim[prim_n];
	for (i=0;i<prim_n;i++) {
		struct draw_primitive *p = &prim[i];
		int index = p->sprite;
		if (d->n >= d->cap) {
			return luaL_error(L, "Too many draw");
		}
		if (index <= 0) {
			++i;
			if (i == prim_n || index == 0)
				return luaL_error(L, "Invalid batch stream");
			i += append_external_material(d, p, (end_ptr - p)/2, index) * 2;
		} else {
			--index;
			if (index >= rect_n)
				return luaL_error(L, "Invalid sprite id %d", index);
			int texid =  rect[index].texid;
			i += append_default_material(d, p, end_ptr - p, texid);
		}
	}

	return 0;
}

static int
ldrawmgr_new(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	void * bank = lua_touserdata(L, 1);
	int cap = luaL_checkinteger(L, 2);
	struct drawmgr * d = (struct drawmgr *)lua_newuserdatauv(L, sizeof(*d) + (cap-1)*sizeof(d->data[0]), 0);
	d->bank = (struct sprite_bank *)bank;
	d->cap = cap;
	d->n = 0;
	if (luaL_newmetatable(L, "SOLUNA_DRAWMGR")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "__len", ldrawmgr_len },
			{ "__call", ldrawmgr_index },
			{ "reset", ldrawmgr_reset },
			{ "append", ldrawmgr_append },
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
luaopen_drawmgr(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", ldrawmgr_new },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
