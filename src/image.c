#include <lua.h>
#include <lauxlib.h>

#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 65536
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

static stbi_uc const *
get_image(lua_State *L, size_t *sz) {
	const char *buffer = luaL_checklstring(L, 1, sz);
	return (stbi_uc const *)buffer;
}

static int
image_load(lua_State *L) {
	size_t sz;
	const stbi_uc *buffer = get_image(L, &sz);
	int x,y,c;
	stbi_uc * img = stbi_load_from_memory(buffer, sz, &x, &y, &c, 4);
	if (img == NULL) {
		lua_pushnil(L);
		lua_pushstring(L, stbi_failure_reason());
		return 2;
	}
	lua_pushlstring(L, (const char *)img, x * y * 4);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	stbi_image_free(img);
	return 3;
};

static int
image_info(lua_State *L) {
	size_t sz;
	const stbi_uc* buffer = get_image(L, &sz);
	int x, y, c;
	if (!stbi_info_from_memory(buffer, sz, &x, &y, &c)) {
		lua_pushnil(L);
		lua_pushstring(L, stbi_failure_reason());
		return 2;
	}
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, c);
	return 3;
}

int
luaopen_image(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "load", image_load },
		{ "info", image_info },
//		{ "clipbox", image_clipbox },
//		{ "blit", image_blit },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
