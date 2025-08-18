#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <stdint.h>

#include "sokol/sokol_gfx.h"
#include "maskquad.glsl.h"
#include "srbuffer.h"
#include "batch.h"
#include "spritemgr.h"
#include "material_util.h"

#define BATCHN 4096

struct color {
	unsigned char channel[4];
};

struct inst_object {
	float x, y;
	float sr_index;
	struct color maskcolor;
	uint32_t offset;
	uint32_t u;
	uint32_t v;
};

struct mask {
	struct draw_primitive_external header;
	struct color c;
};

struct buffer_data {
	struct inst_object inst[BATCHN];
};

struct material_mask {
	sg_pipeline pip;
	sg_buffer inst;
	sg_bindings *bind;
	vs_params_t *uniform;
	struct sr_buffer *srbuffer;
	struct sprite_bank *bank;
};

static void
submit(lua_State *L, struct material_mask *m, struct draw_primitive *prim, int n) {
	struct sprite_rect *rect = m->bank->rect;
	struct buffer_data tmp;
	int i;
	for (i=0;i<n;i++) {
		struct draw_primitive *p = &prim[i*2];
		assert(p->sprite == -MATERIAL_MASK);
		
		struct mask * mask = (struct mask *)&prim[i*2+1];

		// calc scale/rot index
		int sr_index = srbuffer_add(m->srbuffer, p->sr);
		if (sr_index < 0) {
			// todo: support multiply srbuffer
			luaL_error(L, "sr buffer is full");
		}
		tmp.inst[i].x = (float)p->x / 256.0f;
		tmp.inst[i].y = (float)p->y / 256.0f;
		tmp.inst[i].sr_index = (float)sr_index;
		tmp.inst[i].maskcolor = mask->c;
		
		int index = mask->header.sprite;
		assert(index >= 0);
		struct sprite_rect *r = &rect[index];
		tmp.inst[i].offset = r->off;
		tmp.inst[i].u = r->u;
		tmp.inst[i].v = r->v;
	}
	sg_append_buffer(m->inst, &(sg_range) { tmp.inst , n * sizeof(tmp.inst[0]) });
}

static int
lmaterial_mask_submit(lua_State *L) {
	struct material_mask *m = (struct material_mask *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_MASK");
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
lmaterial_mask_draw(lua_State *L) {
	struct material_mask *m = (struct material_mask *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_MASK");
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
init_pipeline(struct material_mask *p) {
	sg_shader shd = sg_make_shader(maskquad_shader_desc(sg_query_backend()));

	p->pip = sg_make_pipeline(&(sg_pipeline_desc) {
		.layout = {
			.buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
			.attrs = {
					[ATTR_maskquad_position].format = SG_VERTEXFORMAT_FLOAT3,
					[ATTR_maskquad_color].format = SG_VERTEXFORMAT_UBYTE4N,
					[ATTR_maskquad_offset].format = SG_VERTEXFORMAT_UINT,
					[ATTR_maskquad_u].format = SG_VERTEXFORMAT_UINT,
					[ATTR_maskquad_v].format = SG_VERTEXFORMAT_UINT,
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
        .label = "mask-pipeline"
    });
}

static int
lnew_material_mask(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct material_mask *m = (struct material_mask *)lua_newuserdatauv(L, sizeof(*m), 4);
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
	
	if (luaL_newmetatable(L, "SOLUNA_MATERIAL_MASK")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "submit", lmaterial_mask_submit },
			{ "draw", lmaterial_mask_draw },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

struct mask_primitive {
	struct draw_primitive pos;
	union {
		struct draw_primitive dummy;
		struct mask m;
	} u;
};

static int
lmask(lua_State *L) {
	struct mask_primitive prim;
	prim.pos.x = 0;
	prim.pos.y = 0;
	prim.pos.sr = 0;
	prim.pos.sprite = -MATERIAL_MASK;
	prim.u.m.header.sprite = luaL_checkinteger(L, 1) - 1;
	uint32_t color = luaL_checkinteger(L, 2);
	if (!(color & 0xff000000))
		color |= 0xff000000;
	prim.u.m.c.channel[0] = (color >> 16) & 0xff;
	prim.u.m.c.channel[1] = (color >> 8) & 0xff;
	prim.u.m.c.channel[2] = color & 0xff;
	prim.u.m.c.channel[3] = (color >> 24) & 0xff;
	lua_pushlstring(L, (const char *)&prim, sizeof(prim));
	return 1;
}

int
luaopen_material_mask(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "mask", lmask },
		{ "new", lnew_material_mask },
		{ "instance_size", NULL },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	
	lua_pushinteger(L, sizeof(struct inst_object));
	lua_setfield(L, -2, "instance_size");
	return 1;
}
