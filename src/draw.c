#include "draw.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_app.h"
#include "sprite_submit.h"
#include "texquad.glsl.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define MAX_SPRITE_VERTEX 4096

struct vertex_t {
	float dx, dy;
	float u, v;
};

struct instance_t {
	float x, y;
	float sr;
};

void
draw_state_init(struct draw_state *state, int w, int h) {
	state->frame[0] = 2.0f / (float)w;
	state->frame[1] = -2.0f / (float)h;
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

void
draw_state_commit(struct draw_state *state) {
	srbuffer_init(&state->srb_mem);
	struct draw_primitive tmp;
	uint64_t count = sapp_frame_count();
	sprite_set_rot(&tmp, count * 3.1415927f / 180.0f);
	int index1 = srbuffer_add(&state->srb_mem, tmp.sr);
	sprite_set_rot(&tmp, -(count * 3.1415927f / 180.0f));
	int index2 = srbuffer_add(&state->srb_mem, tmp.sr);
	assert(index1 <= 2);
	assert(index2 <= 2);
	int x = 256, y = 256;
	int length = 128;
	struct vertex_t vertices[] = {
		{  -length,  -length,     0,    0 },
		{   length,  -length,     1,    0 },
		{  -length,   length,     0,    1 },
		{   length,   length,     1,    1 },
		{  -length/2,  -length/2, 0,    0 },
		{   length/2,  -length/2, 1,    0 },
		{  -length/2,   length/2, 0,    1 },
		{   length/2,   length/2, 1,    1 },
	};
	struct instance_t inst[] = {
		{ x, y, (float)index1 },
		{ x+100, y+100, (float)index2 },
	};
	sg_update_buffer(state->bind.vertex_buffers[0], &SG_RANGE(inst));
	sg_update_buffer(state->bind.storage_buffers[SBUF_sr_lut], &(sg_range){ state->srb_mem.data, state->srb_mem.n * sizeof(struct sr_mat)});
	sg_update_buffer(state->bind.storage_buffers[SBUF_sprite_buffer], &SG_RANGE(vertices));

	vs_params_t vs_params;
	memcpy(vs_params.framesize, state->frame, sizeof(state->frame));
	sg_begin_pass(&(sg_pass) { .action = state->pass_action, .swapchain = sglue_swapchain() });
	sg_apply_pipeline(state->pip);
	sg_apply_bindings(&state->bind);
	sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
	sg_draw(0, 4, 2);
	sg_end_pass();
	sg_commit();	
}
