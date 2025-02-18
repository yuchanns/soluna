#ifndef soluna_loginfo_h
#define soluna_loginfo_h

#include <stdint.h>

struct log_info {
	char tag[64];
	uint32_t log_level;
	uint32_t log_item;
	uint32_t line_nr;
	char message[256];
	const char *filename;	
};

#endif