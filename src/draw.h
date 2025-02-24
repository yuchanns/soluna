#ifndef soluna_draw_h
#define soluna_draw_h

#include "sokol/sokol_gfx.h"
#include "srbuffer.h"

struct draw_state {
	sg_pipeline pip;
	sg_bindings bind;
	struct sr_buffer srb_mem;
	float texsize[2];
	float frame[2];
};

#endif
