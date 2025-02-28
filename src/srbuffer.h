#ifndef soluna_srbuffer_h
#define soluna_srbuffer_h

#include <stdint.h>

struct sr_mat {
	float v[4];
};

struct sr_buffer {
	int n;
	int cap;
	int current_n;
	uint8_t dirty;
	uint8_t current_frame;
	uint8_t *frame;
	uint16_t *cache;
	uint32_t *key;
	struct sr_mat *data;
};

size_t srbuffer_size(int n);
void srbuffer_init(struct sr_buffer *SR, int n);
int srbuffer_add(struct sr_buffer *SR, uint32_t sr);
void * srbuffer_commit(struct sr_buffer *SR, int *sz);

#endif
