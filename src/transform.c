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
	int64_t x, y;
	
	if (t->r != 0) {
		int sin, cos;
		sincos_lut(t->r, &sin, &cos);

		int64_t x0 = p->x;
		int64_t y0 = p->y;
		y = y0 * cos + x0 * sin;
		x = x0 * cos - y0 * sin;
		
		x >>= 24;
		y >>= 24;

		int r = p->sr & 0xfff;
		r = (r + t->r) % 4096;
		p->sr = (p->sr & ~0xfff) | r;
	} else {
		x = p->x;
		y = p->y;
	}

	if (t->s != 0x1000) {
		x *= t->s;	// .(12 + 12) fix number
		y *= t->s;
		x >>= 12;
		y >>= 12;
		sprite_apply_scale(p, t->s);
	}
	p->x = (int32_t)x + t->x;
	p->y = (int32_t)y + t->y;
}

void
sprite_transform_set(struct transform *t, float s, float r, float x, float y) {
	t->s = (int32_t)(s * 4096);
	const float rot_scale = 2048.0 / 3.1415927;
	t->r = (int)(r * rot_scale) % 4096;
	t->x = (int32_t)(x * 256);
	t->y = (int32_t)(y * 256);
}

// returns origin point (x, y) in transformed coordinate system
void
sprite_transform_point(const struct transform *t, int *x, int *y) {
	int64_t ox = *x;
	int64_t oy = *y;

	ox -= t->x;
	oy -= t->y;
	if (t->r != 0) {
		int sin, cos;
		sincos_lut(t->r, &sin, &cos);

		int64_t x = ox;
		int64_t y = oy;
		ox = x * cos + y * sin;
		oy = y * cos - x * sin;
		ox >>= 24;
		oy >>= 24;
	}
	if (t->s != 4096) {
		int s = t->s;
		if (s == 0)
			s = 1;
		int32_t inv_s = (1 << 30) / s;	// .18bits fix
		ox *= inv_s;
		oy *= inv_s;
		
		ox >>= 18;
		oy >>= 18;
	}
	*x = (int)ox;
	*y = (int)oy;
}
