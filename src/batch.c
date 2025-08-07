#include "batch.h"
#include <stdlib.h>
#include <assert.h>
#include <stdatomic.h>

#define DEFAULT_SIZE 1024

struct draw_batch {
	int cap;
	struct draw_primitive * stream;
};

struct draw_batch *
batch_new(int size) {
	if (size < DEFAULT_SIZE)
		size = DEFAULT_SIZE;
	struct draw_batch * batch = (struct draw_batch *)malloc(sizeof(*batch));
	if (batch == NULL)
		return NULL;
	batch->cap = size;
	batch->stream = (struct draw_primitive *)malloc(sizeof(struct draw_primitive) * size);
	if (batch->stream == NULL) {
		free(batch);
		return NULL;
	}
	return batch;
}

void
batch_delete(struct draw_batch *B) {
	if (B == NULL)
		return;
	free(B->stream);
	free(B);
}

struct draw_primitive *
batch_reserve(struct draw_batch *B, int size) {
	if (size <= B->cap)
		return B->stream;
	int cap = B->cap;
	do {
		cap = cap * 3/2;
	} while (cap < size);
	struct draw_primitive * stream = realloc(B->stream, cap * sizeof(struct draw_primitive));
	if (stream == NULL)
		return NULL;
	B->stream = stream;
	B->cap = cap;
	return stream;
}
