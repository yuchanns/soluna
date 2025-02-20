#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>

#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 65536
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

static stbi_uc const *
get_buffer(lua_State *L, size_t *sz) {
	const char *buffer = luaL_checklstring(L, 1, sz);
	return (stbi_uc const *)buffer;
}

static int
image_load(lua_State *L) {
	size_t sz;
	const stbi_uc *buffer = get_buffer(L, &sz);
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
	const stbi_uc* buffer = get_buffer(L, &sz);
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

struct rect {
	const uint8_t * ptr;
	int stride;
	int width;
	int line;
};

static int
rect_init(struct rect *r, const uint8_t *buffer, int x, int y, int dx, int dy, int w, int h) {
	if (dx < 0) {
		w += dx;
		dx = 0;
	} else if (dx > x) {
		return 0;
	}
	if (dy < 0) {
		h += dy;
		dy = 0;
	} else if (dy > y) {
		return 0;
	}
	if (w + dx > x) {
		w = x - dx;
	}
	if (h + dy > y) {
		h = y - dy;
	}
	if (w <=0 || h <= 0)
		return 0;
	
	r->ptr = buffer + x * 4 * dy + 4 * dx;
	r->stride = 4 * x;
	r->width = w;
	r->line = h;

	return 1;
}

static int
remove_top(struct rect *r) {
	int x, y;
	const uint8_t * ptr = r->ptr;
	for (y=0;y<r->line;y++) {
		const uint8_t *cur = ptr;
		for (x=0;x<r->width;x++) {
			if (cur[3]) {
				r->ptr = ptr;
				r->line -= y;
				return y;
			}
			cur += 4;
		}
		ptr += r->stride;
	}
	return r->line;
}

static int
remove_bottom(struct rect *r) {
	int x, y;
	const uint8_t * ptr = r->ptr + (r->line - 1) * r->stride;
	for (y=0;y<r->line-1;y++) {
		const uint8_t *cur = ptr;
		for (x=0;x<r->width;x++) {
			if (cur[3]) {
				r->line -= y;
				return y;
			}
			cur += 4;
		}
		ptr -= r->stride;
	}
	return r->line;
}

static int
remove_left(struct rect *r) {
	int x, y;
	const uint8_t * ptr = r->ptr;
	int min_left = r->width;
	for (y=0;y<r->line;y++) {
		const uint8_t *cur = ptr;
		for (x=0;x<min_left;x++) {
			if (cur[3]) {
				break;
			}
			cur += 4;
		}
		if (x == 0)
			return 0;
		else if (x < min_left) {
			min_left = x;
		}
		ptr += r->stride;
	}
	return min_left;
}

static int
remove_right(struct rect *r) {
	int x, y;
	const uint8_t * ptr = r->ptr + r->width * 4;
	int min_right = r->width;
	for (y=0;y<r->line;y++) {
		const uint8_t *cur = ptr;
		for (x=0;x<min_right;x++) {
			cur -= 4;
			if (cur[3]) {
				break;
			}
		}
		if (x == 0)
			return 0;
		else if (x < min_right) {
			min_right = x;
		}
		ptr += r->stride;
	}
	return min_right;
}

static int
image_clipbox(lua_State *L) {
	size_t sz;
	const uint8_t * image = get_buffer(L, &sz);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	if (x * y * 4 != sz)
		return luaL_error(L, "Invalid image size %d * %d * 4 != %z\n", x, y, sz);
	int dx = luaL_optinteger(L, 4, 0);
	int dy = luaL_optinteger(L, 5, 0);
	int w = luaL_optinteger(L, 6, x - dx);
	int h = luaL_optinteger(L, 7, y - dy);
	
	struct rect r;
	
	if (!(rect_init(&r, image, x, y, dx, dy, w, h))) {
		return 0;
	}
	
	int top = remove_top(&r);
	if (top == h)
		return 0;
	int bottom = remove_bottom(&r);
	int left = remove_left(&r);
	int right = remove_right(&r);
	
	lua_pushinteger(L, left);
	lua_pushinteger(L, top);
	lua_pushinteger(L, w - (left + right));
	lua_pushinteger(L, h - (top + bottom));

	return 4;
}

int
luaopen_image(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "load", image_load },
		{ "info", image_info },
		{ "clipbox", image_clipbox },
//		{ "blit", image_blit },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
