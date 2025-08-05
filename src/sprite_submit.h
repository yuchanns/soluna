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
		// scale is 12+8 bits fixpoint
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
sprite_apply_scale(struct draw_primitive *p, uint32_t scale_fix, uint32_t inv_scale) {
	if (scale_fix == 0x1000)
		return;
	int64_t x = p->x;
	int64_t y = p->y;
	x = (x * inv_scale) >> 19;
	y = (y * inv_scale) >> 19;
	p->x = (int32_t)x;
	p->y = (int32_t)y;
	
	uint32_t orig_scale = p->sr >> 12;
	if (orig_scale != 0) {
		if (orig_scale >= 0xff000) {
			orig_scale &= 0xfff;
		} else {
			orig_scale = (orig_scale + 256) << 4;
		}
		uint64_t tmp = orig_scale;
		tmp *= scale_fix;
		scale_fix = (uint32_t)(tmp >> 12);
	}
	if (scale_fix < 0x1000) {
		scale_fix |= 0xff000;
	} else {
		scale_fix -= 0x1000;
		scale_fix >>= 8;
		if (scale_fix > 0xfefff)
			scale_fix = 0xfefff;
	}
	p->sr = (scale_fix << 12) | (p->sr & 0xfff);
}

#endif