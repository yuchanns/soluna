#include <lua.h>
#include <lauxlib.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#include <windows.h>

static void
open_url(lua_State *L, const char *url) {
	int n = MultiByteToWideChar(CP_UTF8, 0, url, -1, NULL, 0);
	if (n == 0)
		luaL_error(L, "Invalid url string : %s", url);
	void * buf = lua_newuserdatauv(L, n * sizeof(WCHAR), 0);
	MultiByteToWideChar(CP_UTF8, 0, url, -1, (WCHAR *)buf, n);
	ShellExecuteW(NULL, L"open", (WCHAR *)buf, NULL, NULL, SW_SHOWNORMAL);
}

#else

static void
open_url(lua_State *L, const char *url) {
// todo : support mac/linux
}

#endif

static int
lurl_open(lua_State *L) {
	const char * url = luaL_checkstring(L, 1);
	open_url(L, url);
	return 0;
}

int
luaopen_url(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "open", lurl_open },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}