#include "batch.h"
#include <stdlib.h>
#include <assert.h>
#include <stdatomic.h>

#define DEFAULT_SIZE 1024
#define MAX_BATCH 1024

struct batch_ref {
	int n;
	int freelist;
	union {
		atomic_int ref[MAX_BATCH];
		int next[MAX_BATCH];
	} u;
};

static struct batch_ref BREF;

static int
batch_id_alloc(struct batch_ref *B) {
	int free_index;
	if (B->freelist) {
		free_index = B->freelist - 1;
		B->freelist = B->u.next[free_index];
	} else {
		free_index = B->n;
		if (free_index >= MAX_BATCH)
			return 0;
		++B->n;
	}
	atomic_init(&B->u.ref[free_index], 0);
	return free_index + 1;
}

static void
batch_id_dealloc(struct batch_ref *B, int id) {
	assert(id > 0 && id <= MAX_BATCH);
	int index = id - 1;
	assert(B->u.next[index] == 0);
	B->u.next[index] = B->freelist;
	B->freelist = index;
}

static inline void
batch_id_grab(struct batch_ref *B, int id) {
	assert(id > 0 && id <= MAX_BATCH);
	int index = id - 1;
	atomic_fetch_add(&B->u.ref[index], 1);
}

static inline int
batch_id_release(struct batch_ref *B, int id) {
	assert(id > 0 && id <= MAX_BATCH);
	int index = id - 1;
	return atomic_fetch_sub(&B->u.ref[index], 1) - 1;
}

static inline int
batch_id_busy(struct batch_ref *B, int id) {
	assert(id > 0 && id <= MAX_BATCH);
	int index = id - 1;
	return atomic_load(&B->u.ref[index]);
}

struct draw_batch *
batch_new(int size) {
	if (size < DEFAULT_SIZE)
		size = DEFAULT_SIZE;
	int id = batch_id_alloc(&BREF);
	if (id == 0)
		return NULL;
	struct draw_batch * batch = (struct draw_batch *)malloc(sizeof(*batch));
	if (batch == NULL)
		return NULL;
	batch->id = id;
	batch->n = 0;
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
	batch_id_dealloc(&BREF, B->id);
	free(B->stream);
	free(B);
}

void
batch_grab(struct draw_batch *B) {
	batch_id_grab(&BREF, B->id);
}

int
batch_release(struct draw_batch *B) {
	return batch_id_release(&BREF, B->id);
}

int
batch_busy(struct draw_batch *B) {
	return batch_id_busy(&BREF, B->id);
}

struct draw_primitive *
batch_reserve(struct draw_batch *B, int size) {
	if (size <= B->cap)
		return B->stream;
	int cap = B->cap;
	do {
		cap = cap * 3/2;
	} while (cap >= size);
	struct draw_primitive * stream = realloc(B->stream, cap * sizeof(struct draw_primitive));
	if (stream == NULL)
		return NULL;
	B->stream = stream;
	B->cap = cap;
	return stream;
}
