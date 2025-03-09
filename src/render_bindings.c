#include <lua.h>
#include <lauxlib.h>
#include <string.h>

#include "sokol/sokol_gfx.h"

static int
lbindings_set_vb(lua_State *L) {
	sg_bindings *b = luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	int index = luaL_checkinteger(L, 2);
	if (index < 0 || index >= SG_MAX_VERTEXBUFFER_BINDSLOTS)
		return luaL_error(L, "Invalid vbuffer slot %d", index);
	luaL_checkudata(L, 3, "SOKOL_BUFFER");
	lua_settop(L, 3);
	lua_pushlightuserdata(L, &b->vertex_buffers[index]);
	lua_call(L, 1, 0);
	return 0;
}

static int
lbindings_set_voff(lua_State *L) {
	sg_bindings *b = luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	int index = luaL_checkinteger(L, 2);
	if (index < 0 || index >= SG_MAX_VERTEXBUFFER_BINDSLOTS)
		return luaL_error(L, "Invalid vbuffer slot %d", index);
	b->vertex_buffer_offsets[index] = luaL_checkinteger(L, 3);
	return 0;
}

static int
lbindings_set_ib(lua_State *L) {
	sg_bindings *b = luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	luaL_checkudata(L, 2, "SOKOL_BUFFER");
	lua_settop(L, 2);
	lua_pushlightuserdata(L, &b->index_buffer);
	lua_call(L, 1, 0);
	return 0;
}

static int
lbindings_set_ioff(lua_State *L) {
	sg_bindings *b = luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	b->index_buffer_offset = luaL_checkinteger(L, 2);
	return 0;
}

static int
lbindings_set_sb(lua_State *L) {
	sg_bindings *b = luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	int index = luaL_checkinteger(L, 2);
	if (index < 0 || index >= SG_MAX_STORAGEBUFFER_BINDSLOTS)
		return luaL_error(L, "Invalid sbuffer slot %d", index);
	luaL_checkudata(L, 3, "SOKOL_BUFFER");
	lua_settop(L, 3);
	lua_pushlightuserdata(L, &b->storage_buffers[index]);
	lua_call(L, 1, 0);
	return 0;
}

static int
lbindings_set_image(lua_State *L) {
	sg_bindings *b = luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	int index = luaL_checkinteger(L, 2);
	if (index < 0 || index >= SG_MAX_IMAGE_BINDSLOTS)
		return luaL_error(L, "Invalid image slot %d", index);
	luaL_checkudata(L, 3, "SOKOL_IMAGE");
	lua_settop(L, 3);
	lua_pushlightuserdata(L, &b->images[index]);
	lua_call(L, 1, 0);
	return 0;
}

static int
lbindings_set_sampler(lua_State *L) {
	sg_bindings *b = luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	int index = luaL_checkinteger(L, 2);
	if (index < 0 || index >= SG_MAX_SAMPLER_BINDSLOTS)
		return luaL_error(L, "Invalid sampler slot %d", index);
	luaL_checkudata(L, 3, "SOKOL_SAMPLER");
	lua_settop(L, 3);
	lua_pushlightuserdata(L, &b->samplers[index]);
	lua_call(L, 1, 0);
	return 0;
}

static int
lbindings_apply(lua_State *L) {
	sg_bindings *b = (sg_bindings *)luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	sg_apply_bindings(b);
	return 0;
}

int
lbindings_new(lua_State *L) {
	sg_bindings * b = (sg_bindings *)lua_newuserdatauv(L, sizeof(*b), 0);
	memset(b, 0, sizeof(*b));
	if (luaL_newmetatable(L, "SOKOL_BINDINGS")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "vbuffer", lbindings_set_vb },
			{ "voffset", lbindings_set_voff },
			{ "ibuffer", lbindings_set_ib },
			{ "ioffset", lbindings_set_ioff },
			{ "sbuffer", lbindings_set_sb },
			{ "image", lbindings_set_image},
			{ "sampler", lbindings_set_sampler },
			{ "apply", lbindings_apply },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}
