#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <stdint.h>

#include "sokol/sokol_gfx.h"
#include "texquad.glsl.h"
#include "srbuffer.h"
#include "batch.h"
#include "spritemgr.h"
#include "material_util.h"

#define BATCHN 4096

struct inst_object {
	float x, y;
	float sr_index;
	uint32_t offset;
	uint32_t u;
	uint32_t v;
};

struct buffer_data {
	struct inst_object inst[BATCHN];
};

struct material_default {
	sg_pipeline pip;
	sg_buffer inst;
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
		tmp.inst[i].offset = r->off;
		tmp.inst[i].u = r->u;
		tmp.inst[i].v = r->v;
	}
	sg_append_buffer(m->inst, &(sg_range) { tmp.inst , n * sizeof(tmp.inst[0]) });
}

static int
lmaterial_default_submit(lua_State *L) {
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
lmaterial_default_draw(lua_State *L) {
	struct material_default *m = (struct material_default *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_DEFAULT");
//	struct draw_primitive *prim = lua_touserdata(L, 2);
	int prim_n = luaL_checkinteger(L, 3);
//	int tex_id = luaL_checkinteger(L, 4);

	sg_apply_pipeline(m->pip);
	sg_apply_uniforms(UB_vs_params, &(sg_range){ m->uniform, sizeof(vs_params_t) });
	sg_apply_bindings(m->bind);
	sg_draw(0, 4, prim_n);
	
	m->bind->vertex_buffer_offsets[0] += prim_n * sizeof(struct inst_object);

	return 0;
}

static void
init_pipeline(struct material_default *p) {
	sg_shader shd = sg_make_shader(texquad_shader_desc(sg_query_backend()));

	p->pip = sg_make_pipeline(&(sg_pipeline_desc) {
		.layout = {
			.buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
			.attrs = {
					[ATTR_texquad_position].format = SG_VERTEXFORMAT_FLOAT3,
					[ATTR_texquad_offset].format = SG_VERTEXFORMAT_UINT,
					[ATTR_texquad_u].format = SG_VERTEXFORMAT_UINT,
					[ATTR_texquad_v].format = SG_VERTEXFORMAT_UINT,
				}
        },
		.colors[0].blend = (sg_blend_state) {
			.enabled = true,
			.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
			.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.src_factor_alpha = SG_BLENDFACTOR_ONE,
			.dst_factor_alpha = SG_BLENDFACTOR_ZERO
		},
        .shader = shd,
		.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .label = "default-pipeline"
    });
}

static int
lnew_material_default(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct material_default *m = (struct material_default *)lua_newuserdatauv(L, sizeof(*m), 4);
	init_pipeline(m);
	ref_object(L, &m->inst, 1, "inst_buffer", "SOKOL_BUFFER", 0);
	ref_object(L, &m->bind, 2, "bindings", "SOKOL_BINDINGS", 1);
	ref_object(L, &m->uniform, 3, "uniform", "SOKOL_UNIFORM", 1);
	ref_object(L, &m->srbuffer, 4, "sr_buffer", "SOLUNA_SRBUFFER", 1);
	if (lua_getfield(L, 1, "sprite_bank") != LUA_TLIGHTUSERDATA) {
		return luaL_error(L, "Missing .sprite_bank");
	}
	m->bank = lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	if (luaL_newmetatable(L, "SOLUNA_MATERIAL_DEFAULT")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "submit", lmaterial_default_submit },
			{ "draw", lmaterial_default_draw },
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
		{ "instance_size", NULL },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	
	lua_pushinteger(L, sizeof(struct inst_object));
	lua_setfield(L, -2, "instance_size");
	return 1;
}
