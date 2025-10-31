#define linit_c
#define LUA_LIB

#include <stddef.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/threading.h>
#endif

void soluna_embed(lua_State* L);

#if defined(__EMSCRIPTEN__)
static int
em_io_write(lua_State *L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i) {
        size_t len;
        const char *s = luaL_checklstring(L, i, &len);
        for (size_t done = 0; done < len; ) {
            size_t seg = (len - done) > 256 ? 256 : len - done;
            emscripten_outf("%.*s", (int)seg, s + done);
            done += seg;
        }
    }
    lua_pushvalue(L, 1);
    return 1;
}
#endif

void
soluna_openlibs(lua_State *L) {
	// ignore env. vars.
    lua_pushboolean(L, 1);
    lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
	luaL_openlibs(L);
    soluna_embed(L);
#if defined(__EMSCRIPTEN__)
    lua_getglobal(L, "io");
    lua_pushcfunction(L, em_io_write);
    lua_setfield(L, -2, "write");
    lua_pop(L, 1);
#endif
}
