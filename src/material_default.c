#include <lua.h>
#include <lauxlib.h>
#include <assert.h>

#include "sokol/sokol_gfx.h"
#include "texquad.glsl.h"
#include "srbuffer.h"
#include "batch.h"
#include "spritemgr.h"

#define BATCHN 4096

struct inst_object {
	float x, y;
	float sr_index;
};

struct buffer_data {
	sprite_t spr[BATCHN];
	struct inst_object inst[BATCHN];
};

struct material_default {
	sg_pipeline pip;
	sg_buffer inst;
	sg_buffer sprite;
	sg_bindings *bind;
	vs_params_t *uniform;
	struct sr_buffer *srbuffer;
	struct sprite_bank *bank;
};

static void
submit(lua_State *L, struct material_default *m, struct draw_primitive *prim, int n) {
	struct sprite_rect *rect = m->bank->rect;
	struct buffer_data tmp;
	int i;
	for (i=0;i<n;i++) {
		struct draw_primitive *p = &prim[i];
		
		// calc scale/rot index
		int sr_index = srbuffer_add(m->srbuffer, p->sr);
		if (sr_index < 0) {
			// todo: support multiply srbuffer
			luaL_error(L, "sr buffer is full");
		}
		tmp.inst[i].x = (float)p->x / 256.0f;
		tmp.inst[i].y = (float)p->y / 256.0f;
		tmp.inst[i].sr_index = (float)sr_index;
		
		int index = p->sprite - 1;
		assert(index >= 0);
		struct sprite_rect *r = &rect[index];
		tmp.spr[i].offset = r->off;
		tmp.spr[i].u = r->u;
		tmp.spr[i].v = r->v;
	}
	sg_append_buffer(m->inst, &(sg_range) { tmp.inst , n * sizeof(tmp.inst[0]) });
	sg_append_buffer(m->sprite, &(sg_range) { tmp.spr , n * sizeof(tmp.spr[0]) });
}

static int
lmateraial_default_submit(lua_State *L) {
	struct material_default *m = (struct material_default *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_DEFAULT");
	struct draw_primitive *prim = lua_touserdata(L, 2);
	int prim_n = luaL_checkinteger(L, 3);
	int i;
	for (i=0;i<prim_n;i+=BATCHN) {
		int n = (prim_n - i) % BATCHN;
		submit(L, m, prim, n);
		prim += BATCHN;
	}
	return 0;
}

static int
lmateraial_default_draw(lua_State *L) {
	struct material_default *m = (struct material_default *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_DEFAULT");
//	struct draw_primitive *prim = lua_touserdata(L, 2);
	int prim_n = luaL_checkinteger(L, 3);
//	int tex_id = luaL_checkinteger(L, 4);

	sg_apply_pipeline(m->pip);
	sg_apply_uniforms(UB_vs_params, &(sg_range){ m->uniform, sizeof(vs_params_t) });
	sg_apply_bindings(m->bind);
	sg_draw(0, 4, prim_n);
	
	m->uniform->baseinst += prim_n;
	m->bind->vertex_buffer_offsets[0] += prim_n * sizeof(struct inst_object);

	return 0;
}

static void
ref_object(lua_State *L, void *ptr, int uv_index, const char *key, const char *luatype, int direct) {
	if (lua_getfield(L, 1, key) != LUA_TUSERDATA)
		luaL_error(L, "Invalid key .%s", key);
	void *obj = luaL_checkudata(L, -1, luatype);
	lua_pushvalue(L, -1);
	// ud, object, object
	lua_setiuservalue(L, -3, uv_index);
	if (!direct) {
		lua_pushlightuserdata(L, ptr);
		lua_call(L, 1, 0);
	} else {
		lua_pop(L, 1);
		void **ref = (void **)ptr;
		*ref = obj;
	}
}

static int
lnew_material_default(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct material_default *m = (struct material_default *)lua_newuserdatauv(L, sizeof(*m), 6);
	ref_object(L, &m->inst, 1, "inst_buffer", "SOKOL_BUFFER", 0);
	ref_object(L, &m->sprite, 2, "sprite_buffer", "SOKOL_BUFFER", 0);
	ref_object(L, &m->bind, 3, "bindings", "SOKOL_BINDINGS", 1);
	ref_object(L, &m->uniform, 4, "uniform", "SOKOL_UNIFORM", 0);
	ref_object(L, &m->pip, 5, "pipeline", "SOKOL_PIPELINE", 0);
	ref_object(L, &m->srbuffer, 6, "sr_buffer", "SOLUNA_SRBUFFER", 1);
	if (lua_getfield(L, 1, "sprite_bank") != LUA_TLIGHTUSERDATA) {
		return luaL_error(L, "Missing .sprite_bank");
	}
	m->bank = lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	if (luaL_newmetatable(L, "SOLUNA_MATERIAL_DEFAULT")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "submit", lmateraial_default_submit },
			{ "draw", lmateraial_default_draw },
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
luaopen_material_default(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", lnew_material_default },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
