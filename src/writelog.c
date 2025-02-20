#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "loginfo.h"

static void
write_timestamp(uint64_t ti) {
	time_t timer = ti / 100;
	int msec = ti % 100;
	char buffer[26];
	struct tm* tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);	
	printf("[%s.%02d]", buffer, msec);
}

static int
log_write_sokol(lua_State *L) {
	static const char *level[] = {
		"PANIC",
		"ERROR",
		"WARN",
		"INFO",
	};
	uint64_t ti = lua_tointeger(L, 1);
	struct log_info *info = (struct log_info *)lua_touserdata(L, 2);
	if (info->log_level > 3)
		info->log_level = 3;
	write_timestamp(ti);
	printf("[%-5s]( %s:%d )", level[info->log_level], info->tag, info->log_item );
	if (info->filename) {
		printf(" %s : (%d)", info->filename, info->line_nr);
	}
	printf(" %s\n", info->message);
	free(info);
	return 0;
}

static int
log_write_ltask(lua_State *L) {
	uint64_t ti = lua_tointeger(L, 1);
	const char *level = luaL_checkstring(L, 2);
	size_t sz;
	const char *msg = luaL_checklstring(L, 3, &sz);
	char upper[6];
	int i;
	for (i=0;i<5;i++) {
		upper[i] = toupper(level[i]);
		if (*level == 0)
			break;
	}
	upper[5] = 0;
	write_timestamp(ti);
	if (strnlen(msg, sz+1) < sz) {
		printf("[%-5s] ", upper);
		int i;
		for (i=0;i<sz;i++) {
			unsigned char c = (unsigned char)msg[i];
			if (c < 32) {
				printf("/%02X", c);
			} else {
				printf("%c", c);
			}
		}
		printf("\n");
	} else {
		printf("[%-5s] %s\n", upper, msg );
	}
	return 0;
}

int
luaopen_applog(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "sokol", log_write_sokol },
		{ "ltask", log_write_ltask },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

