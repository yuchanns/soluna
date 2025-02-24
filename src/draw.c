#include <lua.h>
#include <lauxlib.h>

#include "draw.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sprite_submit.h"
#include "texquad.glsl.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>

struct vertex_t {
	uint32_t offset;
	uint32_t u;
	uint32_t v;
};

#define MAX_SPRITE_VERTEX (64 * 1024 / sizeof(struct vertex_t))

struct instance_t {
	float x, y;
	float sr;
};

static void
draw_state_init(struct draw_state *state, int w, int h) {
	state->frame[0] = 2.0f / (float)w;
	state->frame[1] = -2.0f / (float)h;
	state->texsize[0] = 1.0f / 256.0f;	// texture virtual size
	state->texsize[1] = 1.0f / 256.0f;	// virtual size
	state->bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
		.size = 2 * sizeof(struct instance_t),
		.type = SG_BUFFERTYPE_VERTEXBUFFER,
		.usage = SG_USAGE_STREAM,
		.label = "texquad-instance"
    });
	state->bind.storage_buffers[SBUF_sr_lut] = sg_make_buffer(&(sg_buffer_desc) {
		.size = sizeof(state->srb_mem.data),
		.type = SG_BUFFERTYPE_STORAGEBUFFER,
		.usage = SG_USAGE_DYNAMIC,
		.label = "texquad-scalerot"
	});
	state->bind.storage_buffers[SBUF_sprite_buffer] = sg_make_buffer(&(sg_buffer_desc) {
		.size = sizeof(struct vertex_t) * MAX_SPRITE_VERTEX,
		.type = SG_BUFFERTYPE_STORAGEBUFFER,
		.usage = SG_USAGE_STREAM,
		.label = "texquad-sprite"
	});

	// create a default sampler object with default attributes
    state->bind.samplers[SMP_smp] = sg_make_sampler(&(sg_sampler_desc){
        .label = "texquad-sampler"
    });
	
	sg_shader shd = sg_make_shader(texquad_shader_desc(sg_query_backend()));

	state->pip = sg_make_pipeline(&(sg_pipeline_desc){
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
        .label = "texquad-pipeline"
    });
	srbuffer_init(&state->srb_mem);
}

static inline uint32_t
pack_short2(int16_t a, int16_t b) {
	uint32_t v1 = a + 0x8000;
	uint32_t v2 = b + 0x8000;
	return v1 << 16 | v2;
}

static inline uint32_t
pack_ushort2(uint16_t a, uint16_t b) {
	return a << 16 | b;
}

struct sprite_rect {
	int dx, dy;
	int uv[4];
};

static void
draw_state_commit(struct draw_state *state, struct sprite_rect *rect) {
	srbuffer_init(&state->srb_mem);
	struct draw_primitive tmp;
	uint64_t count = sapp_frame_count();
	float rad = count * 3.1415927f / 180.0f;
	float s = sinf(rad) + 1.2f;
	sprite_set_sr(&tmp, s, rad);
	int index1 = srbuffer_add(&state->srb_mem, tmp.sr);
	sprite_set_sr(&tmp, s, -rad);
	int index2 = srbuffer_add(&state->srb_mem, tmp.sr);
	assert(index1 <= 3);
	assert(index2 <= 3);
	int x = 256, y = 256;
//	printf("dx = %d dy = %d uv = %d %d %d %d\n", rect->dx, rect->dy, rect->uv[0], rect->uv[1], rect->uv[2], rect->uv[3]);
	struct vertex_t vertices[] = {
		{ 
			pack_short2(rect->dx, rect->dy),
			pack_ushort2(rect->uv[0], rect->uv[1]),
			pack_ushort2(rect->uv[2], rect->uv[3]),
		},
//		{ pack_short2(0, 0), pack_ushort2(0, 128), pack_ushort2(0, 128) },
	};
	struct instance_t inst[] = {
		{ x, y, (float)index1 },
//		{ x+100, y+100, (float)index2 },
	};
	sg_update_buffer(state->bind.vertex_buffers[0], &SG_RANGE(inst));
	sg_update_buffer(state->bind.storage_buffers[SBUF_sr_lut], &(sg_range){ state->srb_mem.data, state->srb_mem.n * sizeof(struct sr_mat)});
	sg_update_buffer(state->bind.storage_buffers[SBUF_sprite_buffer], &SG_RANGE(vertices));

	vs_params_t vs_params;
	memcpy(vs_params.texsize, state->texsize, sizeof(state->texsize));
	memcpy(vs_params.framesize, state->frame, sizeof(state->frame));
	sg_apply_pipeline(state->pip);
	sg_apply_bindings(&state->bind);
	sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
	sg_draw(0, 4, sizeof(inst)/sizeof(inst[0]));
}

static int
render_init(lua_State *L) {
	int w = luaL_checkinteger(L, 1);
	int h = luaL_checkinteger(L, 2);
	struct draw_state * S = (struct draw_state *)lua_newuserdatauv(L, sizeof(*S), 0);
	draw_state_init(S, w, h);
	return 1;
}

static int
render_commit(lua_State *L) {
	struct draw_state *S = lua_touserdata(L, 1);
	struct sprite_rect rect;
	rect.dx = luaL_checkinteger(L, 2);
	rect.dy = luaL_checkinteger(L, 3);
	rect.uv[0] = luaL_checkinteger(L, 4);
	rect.uv[1] = rect.uv[0] +luaL_checkinteger(L, 6);
	rect.uv[2] = luaL_checkinteger(L, 5);
	rect.uv[3] = rect.uv[2] + luaL_checkinteger(L, 7);
	draw_state_commit(S, &rect);
	return 0;
}

static int
render_make_image(lua_State *L) {
	struct draw_state *S = lua_touserdata(L, 1);
	void * buffer = lua_touserdata(L, 2);
	int width = luaL_checkinteger(L, 3);
	int height = luaL_checkinteger(L, 4);
	
	S->bind.images[IMG_tex] = sg_make_image(&(sg_image_desc){
		.width = width,
        .height = height,
        .data.subimage[0][0].ptr = buffer,
        .data.subimage[0][0].size = width * height * 4,
        .label = "texquad-texture"
    });
	
	S->texsize[0] = 1.0f / width;
	S->texsize[1] = 1.0f / height;
	return 0;
}

int
luaopen_draw(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "init", render_init },
		{ "commit", render_commit },
		{ "make_image", render_make_image },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
