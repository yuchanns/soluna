#ifndef soluna_batch_h
#define soluna_batch_h

#include <stdint.h>

struct draw_primitive {
	int32_t x;		// sign bit + 23 + 8   fix number
	int32_t y;
	uint32_t sr;	// scale + rot
	int32_t sprite;		// negative : material 
};

struct draw_batch {
	int id;
	int n;
	int cap;
	struct draw_primitive * stream;
};

struct draw_batch * batch_new(int size);
struct draw_primitive * batch_reserve(struct draw_batch *, int size);
void batch_grab(struct draw_batch *);
int batch_release(struct draw_batch *);
int batch_busy(struct draw_batch *);
void batch_delete(struct draw_batch *);

#endif
