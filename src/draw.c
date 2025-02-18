#include "draw.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
//#include "triangle.glsl.h"
#include "texquad.glsl.h"
#include <stdint.h>
#include <string.h>

struct vertex_t {
	float x, y;
	uint16_t u, v;
};

void
draw_state_init(struct draw_state *state, int w, int h) {
	state->frame[0] = 2.0f / (float)w;
	state->frame[1] = -2.0f / (float)h;
	struct vertex_t vertices[] = {
		// pos   // uvs
		{   64,  64,      0,      0 },
		{  256,  64, 0x7fff,      0 },
		{   64, 256,      0, 0x7fff },
		{  256, 256, 0x7fff, 0x7fff },
	};
	state->bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
		.data = SG_RANGE(vertices),
		.label = "texquad-vertices"
    });
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
                [ATTR_texquad_texcoord].format = SG_VERTEXFORMAT_SHORT2N,
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
}

void
draw_state_commit(struct draw_state *state) {
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
