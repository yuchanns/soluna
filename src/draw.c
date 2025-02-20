#include <lua.h>
#include <lauxlib.h>

#include "draw.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
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
	// create a checkerboard texture
	uint32_t pixels[4*4] = {
        0xFFFFFFFF, 0x20000020, 0xFFFFFFFF, 0x20000020,
        0x20000020, 0xFFFFFFFF, 0x20000020, 0xFFFFFFFF,
        0xFFFFFFFF, 0x20000020, 0xFFFFFFFF, 0x20000020,
        0x20000020, 0xFFFFFFFF, 0x20000020, 0xFFFFFFFF,
    };

	// NOTE: SLOT_tex is provided by shader code generation
    state->bind.images[IMG_tex] = sg_make_image(&(sg_image_desc){
        .width = 4,
        .height = 4,
        .data.subimage[0][0] = SG_RANGE(pixels),
        .label = "texquad-texture"
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
    // default pass action
	state->pass_action = (sg_pass_action) {
		.colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.25f, 0.5f, 0.75f, 1.0f } }
	};
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

static void
draw_state_commit(struct draw_state *state) {
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
	int length = 128;
	struct vertex_t vertices[] = {
		{ pack_short2(length, length),     pack_ushort2(0, 256), pack_ushort2(0, 256) },
		{ pack_short2(0, 0), pack_ushort2(0, 128), pack_ushort2(0, 128) },
	};
	struct instance_t inst[] = {
		{ x, y, (float)index1 },
		{ x+100, y+100, (float)index2 },
	};
	sg_update_buffer(state->bind.vertex_buffers[0], &SG_RANGE(inst));
	sg_update_buffer(state->bind.storage_buffers[SBUF_sr_lut], &(sg_range){ state->srb_mem.data, state->srb_mem.n * sizeof(struct sr_mat)});
	sg_update_buffer(state->bind.storage_buffers[SBUF_sprite_buffer], &SG_RANGE(vertices));

	vs_params_t vs_params;
	memcpy(vs_params.texsize, state->texsize, sizeof(state->texsize));
	memcpy(vs_params.framesize, state->frame, sizeof(state->frame));
	sg_begin_pass(&(sg_pass) { .action = state->pass_action, .swapchain = sglue_swapchain() });
	sg_apply_pipeline(state->pip);
	sg_apply_bindings(&state->bind);
	sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
	sg_draw(0, 4, 2);
	sg_end_pass();
	sg_commit();	
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
	draw_state_commit(S);
	return 0;
}

int
luaopen_render(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "init", render_init },
		{ "commit", render_commit },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
