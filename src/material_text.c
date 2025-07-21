#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "sokol/sokol_gfx.h"
#include "sdftext.glsl.h"
#include "srbuffer.h"
#include "batch.h"
#include "spritemgr.h"
#include "font_manager.h"
#include "sprite_submit.h"

#define BATCHN 4096
#define MATERIAL_TEXT_NORMAL 1

struct text {
	int codepoint;
	uint16_t font;
	uint16_t size;
	uint32_t color;
};

struct inst_object {
	float x, y;
	float sr_index;
};

struct buffer_data {
	sprite_t spr[BATCHN];
	struct inst_object inst[BATCHN];
};

struct material_text {
	sg_pipeline pip;
	sg_buffer inst;
	sg_buffer sprite;
	sg_bindings *bind;
	vs_params_t *uniform;
	struct sr_buffer *srbuffer;
	struct font_manager *font;
	fs_params_t fs_uniform;
};

static void
submit(lua_State *L, struct material_text *m, struct draw_primitive *prim, int n) {
	struct buffer_data tmp;
	int i;
	int count = 0;
	for (i=0;i<n;i++) {
		struct draw_primitive *p = &prim[i*2];
		assert(p->sprite == -1);
		
		struct text * t = (struct text *)&prim[i*2+1];
		struct font_glyph g, og;
		const char* err = font_manager_glyph(m->font, t->font, t->codepoint, t->size, &g, &og);
		if (err == NULL) {
			float scale = (float)g.w / og.w;
			tmp.spr[count].offset = (-og.offset_x + 0x8000) << 16 | (-og.offset_y + 0x8000);
			tmp.spr[count].u = og.u << 16 | FONT_MANAGER_GLYPHSIZE;
			tmp.spr[count].v = og.v << 16 | FONT_MANAGER_GLYPHSIZE;

			sprite_mul_scale(p, scale);
			// calc scale/rot index
			int sr_index = srbuffer_add(m->srbuffer, p->sr);
			if (sr_index < 0) {
				// todo: support multiply srbuffer
				luaL_error(L, "sr buffer is full");
			}
			tmp.inst[count].x = (float)p->x / 256.0f;
			tmp.inst[count].y = (float)p->y / 256.0f;
			tmp.inst[count].sr_index = (float)sr_index;
			++count;
		} else {
			t->codepoint = -1;
		}
	}
	sg_append_buffer(m->inst, &(sg_range) { tmp.inst , count * sizeof(tmp.inst[0]) });
	sg_append_buffer(m->sprite, &(sg_range) { tmp.spr , count * sizeof(tmp.spr[0]) });
}

static int
lmateraial_text_submit(lua_State *L) {
	struct material_text *m = (struct material_text *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_TEXT");
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

static void
draw_text(struct material_text *m, uint32_t color, int count) {
	m->fs_uniform.color = color;
	sg_apply_uniforms(UB_vs_params, &(sg_range){ m->uniform, sizeof(vs_params_t) });
	sg_apply_uniforms(UB_fs_params, &(sg_range){ &m->fs_uniform, sizeof(fs_params_t) });
	sg_apply_bindings(m->bind);
	sg_draw(0, 4, count);
	
	m->uniform->baseinst += count;
	m->bind->vertex_buffer_offsets[0] += count * sizeof(struct inst_object);
}

static int
lmateraial_text_draw(lua_State *L) {
	struct material_text *m = (struct material_text *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_TEXT");
	struct draw_primitive *prim = lua_touserdata(L, 2);
	int prim_n = luaL_checkinteger(L, 3);
	if (prim_n <= 0)
		return 0;
	
	int i;
	float texsize = m->uniform->texsize;
	m->uniform->texsize = 1.0f / FONT_MANAGER_TEXSIZE;
	sg_apply_pipeline(m->pip);
	
	int count = -1;
	uint32_t color = 0;
	for (i=0;i<prim_n;i++) {
		struct text * t = (struct text *)&prim[i*2+1];
		if (t->codepoint >= 0) {
			if (count < 0) {
				color = t->color;
				count = 1;
			} else if (t->color != color) {
				draw_text(m, color, count);
				color = t->color;
				count = 1;
			} else {
				++count;
			}
		}
	}
	draw_text(m, color, count);

	m->uniform->texsize = texsize;

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

static void
init_pipeline(struct material_text *m) {
	sg_shader shd = sg_make_shader(texquad_shader_desc(sg_query_backend()));

	m->pip = sg_make_pipeline(&(sg_pipeline_desc) {
		.layout = {
			.buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
			.attrs = {
					[ATTR_texquad_position].format = SG_VERTEXFORMAT_FLOAT3,
				}
        },
		.colors[0].blend = (sg_blend_state) {
			.enabled = true,
			.src_factor_rgb = SG_BLENDFACTOR_ONE,
			.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.src_factor_alpha = SG_BLENDFACTOR_ONE,
			.dst_factor_alpha = SG_BLENDFACTOR_ZERO
		},
        .shader = shd,
		.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .label = "text-pipeline"
    });
}

static int
lnew_material_text_normal(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct material_text *m = (struct material_text *)lua_newuserdatauv(L, sizeof(*m), 5);
	ref_object(L, &m->inst, 1, "inst_buffer", "SOKOL_BUFFER", 0);
	ref_object(L, &m->sprite, 2, "sprite_buffer", "SOKOL_BUFFER", 0);
	ref_object(L, &m->bind, 3, "bindings", "SOKOL_BINDINGS", 1);
	ref_object(L, &m->uniform, 4, "uniform", "SOKOL_UNIFORM", 1);
	ref_object(L, &m->srbuffer, 5, "sr_buffer", "SOLUNA_SRBUFFER", 1);
	init_pipeline(m);

	if (lua_getfield(L, 1, "font_manager") != LUA_TLIGHTUSERDATA) {
		return luaL_error(L, "Missing .font_manager");
	}
	m->font = lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	m->fs_uniform = (fs_params_t) {
		.edge_mask = font_manager_sdf_mask(m->font),
		.dist_multiplier = 1.0f,
		.color= 0xff000000,
	};

	if (luaL_newmetatable(L, "SOLUNA_MATERIAL_TEXT")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "submit", lmateraial_text_submit },
			{ "draw", lmateraial_text_draw },
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
lchar_for_batch(lua_State *L) {
	struct text * t = (struct text *)lua_touserdata(L, lua_upvalueindex(1));
	t->codepoint = luaL_checkinteger(L, 1);
	t->font = luaL_checkinteger(L, 2);
	t->size = luaL_checkinteger(L, 3);
	t->color = luaL_checkinteger(L, 4);
	if (!(t->color & 0xff000000))
		t->color |= 0xff000000;
	lua_pushvalue(L, lua_upvalueindex(1));
	return 1;
}

int
luaopen_material_text(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "char", NULL },
		{ "normal", lnew_material_text_normal },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	
	// char()
	struct text * t = lua_newuserdatauv(L, sizeof(*t), 1);
	memset(t, 0, sizeof(*t));
	lua_pushinteger(L, MATERIAL_TEXT_NORMAL);
	lua_setiuservalue(L, -2, 1);
	lua_pushcclosure(L, lchar_for_batch, 1);
	lua_setfield(L, -2, "char");
	
	return 1;
}
