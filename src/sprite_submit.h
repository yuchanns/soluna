#ifndef soluna_sprite_submit_h
#define soluna_sprite_submit_h

#include "batch.h"
#include <stdint.h>
#include <assert.h>
#include <math.h>

static inline int32_t
to_fixpoint_(float v) {
	v *= 256;
	return (int)v;
}

static inline void
sprite_set_xy(struct draw_primitive *p, float x, float y) {
	p->x = to_fixpoint_(x);
	p->y = to_fixpoint_(y);
}

static inline void
sprite_add_xy(struct draw_primitive *p, float x, float y) {
	p->x += to_fixpoint_(x);
	p->y += to_fixpoint_(y);
}

static inline int
convert_scale_(float scale) {
	assert(scale >= 0);
	if (scale >= 1.0f) {
		// scale is 20 bit fix point
		int fs = (int)((scale - 1.0f) * 256.0f);
		// max scale 1111, 1110, 1111,1111,1111
		const int maxfs = 0xfefff;
		if (fs > maxfs)
			fs = maxfs;
		return fs;
	}
	// use 12 bits for [0,1) scale
	return 0xff000 | (int)(scale * 4096.0f);
}

static inline int
convert_rot_(float rot) {
	const float pi = 3.1415927;
	float v = fmod(rot, 2 * pi);
	if (v < 0) {
		v += 2 * pi;
	}
	return (int)(v * (2048.0f / pi));
}

static inline void
sprite_set_scale(struct draw_primitive *p, float scale) {
	p->sr = convert_scale_(scale) << 12;
}

static inline void
sprite_set_rot(struct draw_primitive *p, float rot) {
	p->sr = convert_rot_(rot);
}

static inline void
sprite_set_sr(struct draw_primitive *p, float scale, float rot) {
	p->sr = convert_scale_(scale) << 12 | convert_rot_(rot);
}

static inline void
sprite_mul_scale(struct draw_primitive *p, float scale) {
	uint32_t scale_fix = p->sr >> 12;
	if (scale_fix == 0) {
		p->sr |= convert_scale_(scale) << 12;
	} else {
		if (scale_fix >= 0xff000) {
			scale *= (float)(scale_fix & 0xfff) * (1.0f / 4096.0f);
		} else {
			scale *= (float)scale_fix * (1.0f / 256.0f) + 1.0f;
		}
		uint32_t sr = p->sr & 0xfff;
		p->sr = sr | (convert_scale_(scale) << 12);
	}
}

#endif