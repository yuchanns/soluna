#include <math.h>
#include "batch.h"
#include "transform.h"
#include "sprite_submit.h"

// sin(0 ~ 2 * pi) * 2^24
static int sin_lut[4096];

void
sprite_transform_init() {
	static int init = 0;
	// Don't card about race condition, because sin_lut is a constant table
	if (init)
		return;
	int i;
	const float pi = 3.1415927f;
	const float pow2 = (float)(1<<24);
	for (i=0;i<4096;i++) {
		sin_lut[i] = (int)(sinf((float)i / 2048.0f * pi) * pow2);
	}
	init = 1;
}

static inline void
sincos_lut(int d, int *sin, int *cos) {
	int x = (unsigned)d;
	*sin = sin_lut[x];
	*cos = sin_lut[(4096 + 1024 - x) % 4096];
}

void
sprite_transform_apply(struct draw_primitive *p, struct transform * t) {
	int64_t x = p->x;
	int64_t y = p->y;
	int rot = p->sr & 0xfff;
	uint32_t scale_fix = p->sr & ~0xfff;
	uint32_t scale_12 = 0x1000;
	
	if (scale_fix != 0) {
		scale_12 = scale_fix >> 12;
		if (scale_12 > 0xff000)
			scale_12 &= 0xfff;
		else {
			scale_12 = (scale_12 + 256) << 4;
		}
		uint32_t inv_s = (1 << 31) / scale_12;
		x = (x * inv_s) >> 19;
		y = (y * inv_s) >> 19;
	}
	
	if (rot != 0) {
		int sin, cos;
		sincos_lut(rot, &sin, &cos);
		int64_t x0 = x;
		int64_t y0 = y;
		
		x = x0 * cos + y0 * sin;
		y = y0 * cos - x0 * sin;
		
		x >>= 24;
		y >>= 24;
	}
	p->x = (int32_t)x + t->x;
	p->y = (int32_t)y + t->y;

	if (t->s != 0x1000) {
		uint64_t tmp = t->s;
		tmp *= scale_12;
		scale_fix = (uint32_t)(tmp >> 12);
		if (scale_fix < 0x1000) {
			scale_fix |= 0xff000;
		} else {
			scale_fix -= 0x1000;
			scale_fix >>= 4;
			if (scale_fix > 0xfefff)
				scale_fix = 0xfefff;
		}
		scale_fix <<= 12;
	}
	
	rot = (uint32_t)(rot + t->r) & 0xfff;
	
	p->sr = scale_fix | rot;
}

void
sprite_transform_set(struct transform *t, float x, float y, float s, float r) {
	t->x = (int32_t)(x * 256);
	t->y = (int32_t)(y * 256);
	t->s = (int32_t)(s * 4096);
	const float rot_scale = 2048.0 / 3.1415927;
	t->r = (int)(r * rot_scale) % 4096;
}
