#ifndef soluna_transform_h
#define soluna_transform_h

#include <stdint.h>

struct transform {
	// ix, iy : sign bit + 23.8   fix number
	int32_t x;	
	int32_t y;
	int32_t s; // sign bit + 19.12 fix number
	int r;	// 12bits [0,4095]
};

struct draw_primitive;

static inline void
sprite_transform_identity(struct transform * t) {
	t->x = 0;
	t->y = 0;
	t->s = 1 << 12;
	t->r = 0;
}

void sprite_transform_init();
void sprite_transform_set(struct transform *t, float s, float r, float x, float y);
void sprite_transform_apply(struct draw_primitive *p, struct transform * t);

#endif
