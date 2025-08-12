#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#include <windows.h>
#include <shlobj.h>

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

static inline int
create_dir_wchar_(const WCHAR *filenameW) {
	WIN32_FIND_DATAW FindFileData;
	HANDLE h = FindFirstFileW(filenameW, &FindFileData);
	if (h == INVALID_HANDLE_VALUE) {
		// create dir
		if (CreateDirectoryW(filenameW, NULL) == 0)
			return 0;
	} else {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			FindClose(h);
			// dir exist
        } else {
            FindClose(h);
			// not a dir
			return 0;
        }
	}
	return 1;
}

static inline int
personal_dir(char name[FILENAME_MAX]) {
	WCHAR filenameW[FILENAME_MAX + 0x200 + 1];
	HRESULT hr = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, filenameW);
	if (hr != S_OK)
		return 0;
	size_t sz = wcslen(filenameW);
	if (sz > FILENAME_MAX)
		return 0;
	wcscpy(filenameW + sz, L"\\My Games");
	sz = wcslen(filenameW);
	if (create_dir_wchar_(filenameW) == 0)
		return 0;
	return WideCharToMultiByte(CP_UTF8, 0, filenameW, sz, name, FILENAME_MAX, NULL, NULL);
}

static inline int
create_dir(const char *dir) {
	WCHAR filenameW[FILENAME_MAX + 0x200 + 1];
	int n = MultiByteToWideChar(CP_UTF8,0,(const char*)dir,-1,filenameW,FILENAME_MAX + 0x200);
	if (n == 0)
		return 0;
	return create_dir_wchar_(filenameW);
}

#define SLASH '\\'

#else

#define SLASH '/'

#define fopen_utf8(f, m) fopen(f, m)

static inline int
personal_dir(char name[FILENAME_MAX]) {
	// todo: support none windows
	return 0;
}

static inline int
create_dir(const char *dir) {
	// todo: support none windows
	return 0;
}

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
	const char *filename = lua_tostring(L, lua_upvalueindex(1));
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
	luaL_checkstring(L, 1);
	lua_pushcclosure(L, loader, 1);
	return 1;
}

static int
lgamedir(lua_State *L) {
	size_t sz;
	const char * dir = luaL_checklstring(L, 1, &sz);
	char name[FILENAME_MAX];
	int n = personal_dir(name);
	if (n == 0)
		return luaL_error(L, "Can't open personal dir");
	if (n + sz + 1 >= FILENAME_MAX) {
		return luaL_error(L, "gamedir name is too long");
	}
	name[n] = SLASH;
	memcpy(name + n + 1, dir, sz+1);
	if (create_dir(name) == 0)
		return luaL_error(L, "create gamedir %s fail", dir);
	lua_pushlstring(L, name, n+sz+1);
	return 1;
}

int
luaopen_soluna_file(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "load", lfile_load },
		{ "loader", lfile_loader },
		{ "gamedir", lgamedir },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
