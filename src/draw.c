#include "draw.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_app.h"
#include "sprite_submit.h"
#include "texquad.glsl.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>

struct vertex_t {
	float x, y;
	int16_t dx, dy;
	uint16_t u, v;
	uint16_t sr;
	uint16_t dummy;
};

void
draw_state_init(struct draw_state *state, int w, int h) {
	state->frame[0] = 2.0f / (float)w;
	state->frame[1] = -2.0f / (float)h;
	state->vb = sg_make_buffer(&(sg_buffer_desc) {
		.size = 4 * sizeof(struct vertex_t),
		.type = SG_BUFFERTYPE_VERTEXBUFFER,
		.usage = SG_USAGE_STREAM,
		.label = "texquad-vertices"
    });
	state->srb = sg_make_buffer(&(sg_buffer_desc) {
		.size = sizeof(state->srb_mem.data),
		.type = SG_BUFFERTYPE_STORAGEBUFFER,
		.usage = SG_USAGE_DYNAMIC,
		.label = "texquad-scalerot"
	});
	state->bind.vertex_buffers[0] = state->vb;
    state->bind.storage_buffers[SBUF_sr_lut] = state->srb;
	uint16_t indices[] = {
		0,1,2, 1,2,3,
	};
    state->bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .data = SG_RANGE(indices),
        .label = "texquad-indices"
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
		.attrs = {
                [ATTR_texquad_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_texquad_offset].format = SG_VERTEXFORMAT_SHORT2N,
                [ATTR_texquad_texcoord].format = SG_VERTEXFORMAT_USHORT4N,
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
        .index_type = SG_INDEXTYPE_UINT16,
        .label = "texquad-pipeline"
    });
    // default pass action
	state->pass_action = (sg_pass_action) {
		.colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.25f, 0.5f, 0.75f, 1.0f } }
	};
	srbuffer_init(&state->srb_mem);
}

void
draw_state_commit(struct draw_state *state) {
	srbuffer_init(&state->srb_mem);
	struct draw_primitive tmp;
	uint64_t count = sapp_frame_count();
	sprite_set_rot(&tmp, count * 3.1415927f / 180.0f);
	int index = srbuffer_add(&state->srb_mem, tmp.sr);
	assert(index <= 1);
	int x = 256, y = 256;
	int length = 128;
	struct vertex_t vertices[] = {
		// pos   // uvs
		{  x,y, -length,  -length,      0,      0, index, 0 },
		{  x,y,  length,  -length, 0xffff,      0, index, 0 },
		{  x,y, -length,   length,      0, 0xffff, index, 0 },
		{  x,y,  length,   length, 0xffff, 0xffff, index, 0 },
	};
	sg_update_buffer(state->vb, &SG_RANGE(vertices));
	sg_update_buffer(state->srb, &(sg_range){ state->srb_mem.data, state->srb_mem.n * sizeof(struct sr_mat)});
	
	vs_params_t vs_params;
	memcpy(vs_params.framesize, state->frame, sizeof(state->frame));
	sg_begin_pass(&(sg_pass) { .action = state->pass_action, .swapchain = sglue_swapchain() });
	sg_apply_pipeline(state->pip);
	sg_apply_bindings(&state->bind);
	sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
	sg_draw(0, 6, 1);
	sg_end_pass();
	sg_commit();	
}
