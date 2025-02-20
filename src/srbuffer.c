#include "srbuffer.h"
#include <math.h>

void
srbuffer_init(struct sr_buffer *SR) {
	SR->n = 1;
	SR->key[0] = 0;
	SR->cache[0] = 0;
	float *v = SR->data[0].v;
	v[0] = 1.0f; v[1] = 0;
	v[2] = 0; v[3] = 1.0f;
}

int
srbuffer_add(struct sr_buffer *SR, uint32_t v) {
	int index = v % (MAX_SR - 1);
	int slot = SR->cache[index];
	if (slot < SR->n && v == SR->key[slot]) {
		return slot;
	}
	if (SR->n >= MAX_SR) {
		int i;
		for (i=1;i<MAX_SR;i++) {
			if (SR->key[i] == v) {
				SR->cache[index] = i;
				return i;
			}
		}
		return -1;	// full
	}
	slot = SR->n++;
	SR->cache[index] = slot;
	SR->key[slot] = v;
	float *mat = SR->data[slot].v;
	uint32_t scale_fix = v >> 12;
	float scale = 1.0f;
	if (scale_fix != 0) {
		if (scale_fix >= 0xff000) {
			scale = (float)(scale_fix & 0xfff) * (1.0f / 4096.0f);
		} else {
			scale = (float)scale_fix * (1.0f / 256.0f) + 1.0f;
		}
	}
	uint32_t rot_fix = v & 0xfff;
	if (rot_fix == 0) {
		mat[0] = scale; mat[1] = 0;
		mat[2] = 0; mat[3] = scale;
	} else {
		const float pi = 3.1415927f;
		float rot = (float) rot_fix * ( pi / 2048.0f );
		float cosr = cosf(rot) * scale;
		float sinr = sinf(rot) * scale;
		mat[0] = cosr; mat[1] = -sinr;
		mat[2] = sinr; mat[3] = cosr;
	}
	return slot;
}

#ifdef TEST_SRBUFFER_MAIN

#include <stdio.h>
#include "sprite_submit.h"

static void
test(float x, float y, float scale, float rad, uint32_t *sr, float *ox, float *oy) {
	struct draw_primitive tmp;
	const float pi = 3.1415927f;
	sprite_set_sr(&tmp, scale, rad * pi / 180.0f);
	struct sr_buffer buffer;
	srbuffer_init(&buffer);
	int index = srbuffer_add(&buffer, tmp.sr);
	const float *mat = buffer.data[index].v;
	*ox = x * mat[0] + y * mat[1];
	*oy = x * mat[2] + y * mat[3];
	*sr = tmp.sr;
}

static void
test_rot(float x, float y, float rot) {
	float ox,oy;
	uint32_t v;
	test(x, y, 1, rot, &v, &ox, &oy);
	printf("[%f,%f / %f] =(%x)=> [%f,%f]\n", x, y, rot, v, ox, oy);
}

static void
test_sr(float x, float y, float scale, float rot) {
	float ox,oy;
	uint32_t v;
	test(x, y, scale, rot, &v, &ox, &oy);
	printf("[%f,%f / %f,%f] =(%x)=> [%f,%f]\n", x, y, scale, rot, v, ox, oy);
}

int
main() {
	test_rot(100, 100, 45);
	test_rot(100, 0, 90);
	test_sr(100, 0, 1.5, 30);
	test_sr(100, 0, 0.5, -60);
	return 0;
}

#endif
