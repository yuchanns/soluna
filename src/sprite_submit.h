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
sprite_apply_xy(struct draw_primitive *p, float x, float y) {
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

static inline uint32_t
convert_scale_part_(uint32_t scale12) {
	uint32_t scale_fix;
	if (scale12 >= 0x1000) {
		scale_fix = (scale12 - 0x1000) >> 4;
		if (scale_fix > 0xfefff)
			scale_fix = 0xfefff;
	} else {
		scale_fix = scale12 | 0xff000;
	}
	return scale_fix << 12;
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
sprite_apply_scale(struct draw_primitive *p, uint32_t scale_fix12) {
	uint32_t scale_fix = p->sr >> 12;
	if (scale_fix == 0) {
		scale_fix = convert_scale_part_(scale_fix12);
	} else {
		uint64_t s;
		if (scale_fix >= 0xff000) {
			s = scale_fix & 0xfff;
		} else {
			s = (scale_fix + 0x100) << 4;
		}
		s = (s * scale_fix12) >> 12;
		scale_fix = convert_scale_part_((uint32_t)s);
	}
	p->sr = scale_fix | (p->sr & 0xfff);
}

#endif