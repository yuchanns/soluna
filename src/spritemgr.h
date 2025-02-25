#ifndef soluna_spritemgr_h
#define soluna_spritemgr_h

#include <stdint.h>
#include <assert.h>

#define INVALID_TEXTUREID 0xffff

struct sprite_rect {
	uint16_t texid;
	uint16_t frame;
	uint32_t off;	// (dx + 0x8000) << (dy + 0x8000)
	uint32_t u;		// x << 16 | w
	uint32_t v;		// y << 16 | h
};

struct sprite_bank {
	int n;
	int cap;
	int texture_size;
	int texture_ready;
	uint16_t texture_n;
	uint16_t current_frame;
	struct sprite_rect rect[1];
};

static inline struct sprite_rect *
sprite_touch(struct sprite_bank *b, int id) {
	assert(id >= 0 && id < b->n);
	struct sprite_rect *rect = &b->rect[id];
	uint16_t f = b->current_frame;
	rect->frame = f;
	if (rect->texid == INVALID_TEXTUREID) {
		rect->texid = 0;
		b->texture_ready = 0;
	} else if (rect->texid == 0) {
		b->texture_ready = 0;
	}
	return rect;
}

#endif
