#ifndef soluna_message_h
#define soluna_message_h

#include <stdint.h>

struct soluna_message {
	const char * type;
	union {
		int p[2];
		uint64_t u64;
	} v;
};

static inline struct soluna_message *
message_create(const char *type, int p1, int p2) {
	struct soluna_message * msg = (struct soluna_message *)malloc(sizeof(*msg));
	msg->type = type;
	msg->v.p[0] = p1;
	msg->v.p[1] = p2;
	return msg;
}

static inline struct soluna_message *
message_create64(const char *type, uint64_t p) {
	struct soluna_message * msg = (struct soluna_message *)malloc(sizeof(*msg));
	msg->type = type;
	msg->v.u64 = p;
	return msg;
}

static inline void
message_release(struct soluna_message *msg) {
	free(msg);
}


#endif
