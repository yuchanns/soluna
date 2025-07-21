#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#include <windows.h>

static inline FILE *
fopen_utf8(const char *filename, const char *mode) {
	WCHAR filenameW[FILENAME_MAX + 0x200 + 1];
	int n = MultiByteToWideChar(CP_UTF8,0,(const char*)filename,-1,filenameW,FILENAME_MAX + 0x200);
	if (n == 0)
		return NULL;
	WCHAR modeW[128];
	n = MultiByteToWideChar(CP_UTF8,0,(const char*)mode,-1,modeW, 127);
	if (n == 0)
		return NULL;
	return _wfopen(filenameW, modeW);
}

#else

#define fopen_utf8(f, m) fopen(f, m)

#endif

static void *
external_free(void *ud, void *ptr, size_t osize, size_t nsize) {
	free(ptr);
	return NULL;
}

static int
lfile_load(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "rb");
	FILE *f = fopen_utf8(filename, mode);
	if (f == NULL)
		return 0;
	fseek(f, 0, SEEK_END);
	size_t sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	char * buffer = (char *)malloc(sz+1);
	if (buffer == NULL) {
		fclose(f);
		return luaL_error(L, "lfile_load_string : Out of memory");
	}
	buffer[sz] = 0;
	size_t rd = fread(buffer, 1, sz, f);
	fclose(f);
	
	if (rd != sz) {
		free(buffer);
		return luaL_error(L, "Read %s fail", filename);
	}
	lua_pushexternalstring(L, buffer, sz, external_free, NULL);
	return 1;
}

struct file_buffer {
	void *ptr;
};

static int
close_buffer(lua_State *L) {
	struct file_buffer *buf = lua_touserdata(L, 1);
	free(buf->ptr);
	buf->ptr = NULL;
	lua_pushnil(L);
	lua_setmetatable(L, 1);
	return 0;
}

static int
loader(lua_State *L) {
	const char *filename = luaL_checkstring(L, lua_upvalueindex(1));
	const char *mode = luaL_optstring(L, 2, "rb");
	FILE *f = fopen_utf8(filename, mode);
	if (f == NULL)
		return luaL_error(L, "Can't open %s", filename);

	struct file_buffer * buf = (struct file_buffer *)lua_newuserdatauv(L, sizeof(*buf), 0);
	buf->ptr = NULL;
	if (luaL_newmetatable(L, "SOLUNA_LOADER")) {
		luaL_Reg l[] = {
			{ "__close", close_buffer },
			{ "__gc", close_buffer },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);
	}
	lua_setmetatable(L, -2);
	
	fseek(f, 0, SEEK_END);
	size_t sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	buf->ptr = malloc(sz);
	if (buf->ptr == NULL) {
		fclose(f);
		return luaL_error(L, "loader : Out of memory");
	}
	size_t rd = fread(buf->ptr, 1, sz, f);
	fclose(f);
	if (rd != sz) {
		free(buf->ptr);
		buf->ptr = NULL;
		return luaL_error(L, "Read %s failed", filename);
	}
	lua_pushlightuserdata(L, buf->ptr);
	lua_pushinteger(L, sz);
	lua_pushvalue(L, -3);
	return 3;
}

static int
lfile_loader(lua_State *L) {
	lua_settop(L, 1);
	lua_pushcclosure(L, loader, 1);
	return 1;
}

int
luaopen_soluna_file(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "load", lfile_load },
		{ "loader", lfile_loader },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
