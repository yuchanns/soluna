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

static int
create_tmp_buffer_(lua_State *L) {
	int sz = lua_tointeger(L, 1);
	lua_newuserdatauv(L, sz, 0);
	return 1;
}

static void *
create_tmp_buffer(lua_State *L, size_t sz) {
	lua_pushcfunction(L, create_tmp_buffer_);
	lua_pushinteger(L, sz);
	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		lua_pop(L, 1);
		return NULL;
	}
	return lua_touserdata(L, -1);
}

static int
push_size_string_(lua_State *L) {
	const char * str = (const char *)lua_touserdata(L, 1);
	size_t sz = lua_tointeger(L, 2);
	lua_pushlstring(L, str, sz);
	return 1;
}

static int
push_size_string(lua_State *L, void *buffer, size_t sz) {
	lua_pushcfunction(L, push_size_string_);
	lua_pushlightuserdata(L, buffer);
	lua_pushinteger(L, sz);
	if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
		lua_pop(L, 1);
		return 0;
	}
	return 1;
}

static int
lfile_load(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "rb");
	FILE *f = fopen_utf8(filename, mode);
	if (f == NULL)
		return luaL_error(L, "Open %s failed", filename);
	fseek(f, 0, SEEK_END);
	size_t sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	void * buffer = create_tmp_buffer(L, sz);
	if (buffer == NULL) {
		fclose(f);
		return luaL_error(L, "Out of memory");
	}
	size_t rd = fread(buffer, 1, sz, f);
	fclose(f);
	if (rd != sz) {
		return luaL_error(L, "Read %s fail", filename);
	}
	return 1;
}

#define FILE_TMPSIZE 65536

static int
lfile_load_string(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "rb");
	FILE *f = fopen_utf8(filename, mode);
	if (f == NULL)
		return luaL_error(L, "Open %s failed", filename);
	fseek(f, 0, SEEK_END);
	size_t sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	char tmp[FILE_TMPSIZE];
	void * buffer = tmp;
	if (sz > FILE_TMPSIZE) {
		buffer = malloc(sz);
		if (buffer == NULL) {
			fclose(f);
			return luaL_error(L, "Out of memory");
		}
	}
	size_t rd = fread(buffer, 1, sz, f);
	fclose(f);
	
	if (sz <= FILE_TMPSIZE) {
		lua_pushlstring(L, buffer, sz);
	} else {
		int succ = push_size_string(L, buffer, sz);
		if (sz > FILE_TMPSIZE)
			free(buffer);
		if (!succ)
			return luaL_error(L, "Out of memory");
	}
	if (rd != sz) {
		return luaL_error(L, "Read %s fail", filename);
	}
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
	return 0;
}

static int
loader(lua_State *L) {
	lua_pushvalue(L, lua_upvalueindex(2));
	lua_pushvalue(L, lua_upvalueindex(3));
	lua_pushvalue(L, lua_upvalueindex(1));
	return 3;
}

static int
lfile_loader(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const char *mode = luaL_optstring(L, 2, "rb");
	FILE *f = fopen_utf8(filename, mode);
	if (f == NULL)
		return luaL_error(L, "Open %s failed", filename);
	
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
		return luaL_error(L, "Out of memory");
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
	lua_pushcclosure(L, loader, 3);
	return 1;
}

int
luaopen_soluna_file(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "load", lfile_load },
		{ "loadstring", lfile_load_string },
		{ "loader", lfile_loader },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
