#include "srbuffer.h"
#include <math.h>

static inline int
pow2(int n) {
	int m = 1 << 5;
	while (m < n) {
		m *= 2;
	}
	return m;
}

size_t
srbuffer_size(int n) {
	struct sr_buffer *dummy = NULL;
	n = pow2(n);
	size_t sz = sizeof(*dummy->frame) +
		sizeof(*dummy->cache) +
		sizeof(*dummy->key) +
		sizeof(*dummy->data);
	return sizeof(*dummy) + sz * n;
}

void
srbuffer_init(struct sr_buffer *SR, int n) {
	n = pow2(n);
	uint8_t *ptr = (uint8_t *)(SR + 1);
	SR->data = (struct sr_mat *)ptr;
	ptr += n * sizeof(SR->data[0]);
	SR->key = (uint32_t *)ptr;
	ptr += n * sizeof(SR->key[0]);
	SR->cache = (uint16_t *)ptr;
	ptr += n * sizeof(SR->cache[0]);
	SR->frame = ptr;

	SR->cap = n;
	SR->n = 1;
	SR->dirty = 1;
	SR->current_frame = 0;
	SR->current_n = 1;
	SR->frame[0] = 0;
	SR->key[0] = 0;
	SR->cache[0] = 0;
	float *v = SR->data[0].v;
	v[0] = 1.0f; v[1] = 0;
	v[2] = 0; v[3] = 1.0f;
}

int
srbuffer_add(struct sr_buffer *SR, uint32_t v) {
	int index = v % (SR->cap - 1);
	int slot = SR->cache[index];
	if (slot < SR->n && v == SR->key[slot]) {
		if (SR->frame[slot] != SR->current_frame) {
			SR->frame[slot] = SR->current_frame;
			++SR->current_n;
		}
		return slot;
	}
	if (SR->n >= SR->cap) {
		int i;
		for (i=1;i<SR->cap;i++) {
			if (SR->key[i] == v) {
				SR->cache[index] = i;
				SR->frame[index] = SR->current_frame;
				return i;
			}
		}
		return -1;	// full
	}
	int new_slot = 1;
	if (SR->current_n * 2 < SR->n) {
		// find an exist slot
		slot = v % SR->n;
		int i;
		for (i=0;i<SR->n;i++) {
			if (SR->frame[slot] != SR->current_frame) {
				SR->frame[slot] = SR->current_frame;
				++SR->current_n;
				new_slot = 0;
				break;
			}
			++slot;
			if (slot >= SR->n)
				slot -= SR->n;
		}
	}
	if (new_slot) {
		slot = SR->n++;
		SR->frame[slot] = SR->current_frame;
		++SR->current_n;
	}
	SR->dirty = 1;
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

void *
srbuffer_commit(struct sr_buffer *SR, int *sz) {
	if (SR->dirty) {
		*sz = SR->n * sizeof(SR->data[0]);
		SR->dirty = 0;
		SR->current_n = 1;
		++SR->current_frame;
		SR->frame[0] = SR->current_frame;
		return SR->data;
	}
	*sz = 0;
	return NULL;
}

#ifdef TEST_SRBUFFER_MAIN

#include <stdio.h>
#include <assert.h>
#include "sprite_submit.h"

static void
test(float x, float y, float scale, float rad, uint32_t *sr, float *ox, float *oy) {
	struct draw_primitive tmp;
	const float pi = 3.1415927f;
	sprite_set_sr(&tmp, scale, rad * pi / 180.0f);
	union {
		struct sr_buffer buffer;
		uint8_t tmp[65535];
	} u;
	assert(srbuffer_size(1024) < sizeof(u.tmp));
	srbuffer_init(&u.buffer, 1024);
	int index = srbuffer_add(&u.buffer, tmp.sr);
	const float *mat = u.buffer.data[index].v;
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
