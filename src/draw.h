#ifndef soluna_draw_h
#define soluna_draw_h

#include "sokol_gfx.h"
#include "srbuffer.h"

struct draw_state {
	sg_pipeline pip;
	sg_bindings bind;
	sg_pass_action pass_action;
	struct sr_buffer srb_mem;
	float texsize[2];
	float frame[2];
};

void draw_state_init(struct draw_state *state, int w, int h);
void draw_state_commit(struct draw_state *state);

#endif
