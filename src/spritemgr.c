#include "spritemgr.h"
#include "sprite_submit.h"
#include "batch.h"

#include <stdint.h>
#include <lua.h>
#include <lauxlib.h>

#define DEFAULT_TEXTURE_SIZE 4096
#define INVALID_TEXTUREID 0xffff

#define MAX_NODE 8192
#define MAX_SPRITE_PER_TEXTURE 65536

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

static int
lbank_add(lua_State *L) {
	struct sprite_bank *b = (struct sprite_bank *)luaL_checkudata(L, 1, "SOLUNA_SPRITEBANK");
	if (b->n >= b->cap) {
		return luaL_error(L, "Too many sprite (%d)", b->n);
	}
	struct sprite_rect *r = &b->rect[b->n++];
	int w = luaL_checkinteger(L, 2);
	int h = luaL_checkinteger(L, 3);
	int dx = luaL_optinteger(L, 4, 0);
	int dy = luaL_optinteger(L, 5, 0);
	if (w <= 0 || w > 0xffff || h <=0 || h >= 0xffff)
		return luaL_error(L, "Invalid sprite size (%d * %d)", w, h);
	if (dx < -0x8000 || dx > 0x7fff || dy < -0x8000 || dy > 0x7ffff)
		return luaL_error(L, "Invalid sprite offset (%d * %d)", dx, dy);
	r->u = w;
	r->v = h;
	r->off = (dx + 0x8000) << 16 | (dy + 0x8000);
	r->texid = INVALID_TEXTUREID;
	r->frame = 0;
	lua_pushinteger(L, b->n);
	return 1;
}

static int
lbank_touch(lua_State *L) {
	struct sprite_bank *b = (struct sprite_bank *)luaL_checkudata(L, 1, "SOLUNA_SPRITEBANK");
	int id = luaL_checkinteger(L, 2) - 1;
	if (id < 0 || id >= b->n)
		return luaL_error(L, "Invalid sprite id %d", id);
	sprite_touch(b, id);
	return 0;
}

static int
pack_sprite(struct sprite_bank *b, stbrp_context *ctx, stbrp_node *tmp, stbrp_rect *srect, int from, int reserved, int *reserved_n) {
	int current_frame = b->current_frame;
	int last_texid = b->texture_n;
	
	stbrp_init_target(ctx, b->texture_size, b->texture_size, tmp, MAX_NODE);
	int i;
	int rect_i = reserved;
	for (i=from;i<b->n;i++) {
		struct sprite_rect *rect = &b->rect[i];
		if ((rect->texid == 0 || rect->texid == last_texid) && rect->frame == current_frame) {
			if (rect_i >= MAX_SPRITE_PER_TEXTURE)
				break;
			stbrp_rect * sr = &srect[rect_i++];
			sr->id = i;
			sr->w = rect->u & 0xffff;
			sr->h = rect->v & 0xffff;
		}
	}
	if (stbrp_pack_rects(ctx, srect, rect_i)) {
		// succ
		int j;
		for (j=0;j<rect_i;j++) {
			stbrp_rect * sr = &srect[j];
			struct sprite_rect *rect = &b->rect[sr->id];
			rect->u = sr->x << 16 | sr->w;
			rect->v = sr->y << 16 | sr->h;
			rect->texid = last_texid;
		}
		*reserved_n = 0;
	} else {
		// pack a part
		int j;
		int n = 0;
		for (j=0;j<rect_i;j++) {
			stbrp_rect * sr = &srect[j];
			struct sprite_rect *rect = &b->rect[sr->id];
			if (sr->was_packed) {
				rect->u = sr->x << 16 | sr->w;
				rect->v = sr->y << 16 | sr->h;
				rect->texid = last_texid;
			} else {
				stbrp_rect * tmp = &srect[n];
				tmp->w = sr->w;
				tmp->h = sr->h;
				tmp->id = sr->id;
				++n;
			}
		}
		*reserved_n = n;
	}
	return i;
}

static int
lbank_pack(lua_State *L) {
	struct sprite_bank *b = (struct sprite_bank *)luaL_checkudata(L, 1, "SOLUNA_SPRITEBANK");
	if (b->texture_ready) {
		++b->current_frame;
		return 0;
	}

	stbrp_context ctx;
	stbrp_node tmp[MAX_NODE];
	stbrp_rect rect[MAX_SPRITE_PER_TEXTURE];
	
	int texture = b->texture_n;
	int from = 0;
	int reserved = 0;
	for (;;) {
		from = pack_sprite(b, &ctx, tmp, rect, from, reserved, &reserved);
		if (reserved == 0 && from >= b->n) {
			break;
		}
		++b->texture_n;
	}
	b->texture_ready = 1;
	
	lua_pushinteger(L, texture);
	lua_pushinteger(L, b->texture_n - texture + 1);

	++b->current_frame;
	return 2;
}

static int
lbank_altas(lua_State *L) {
	struct sprite_bank *b = (struct sprite_bank *)luaL_checkudata(L, 1, "SOLUNA_SPRITEBANK");
	int tid = luaL_checkinteger(L, 2);
	int i;
	lua_newtable(L);
	for (i=0;i<b->n;i++) {
		struct sprite_rect *rect = &b->rect[i];
		if (rect->texid == tid) {
			uint64_t x = rect->u >> 16;
			uint64_t y = rect->v >> 16;
			uint64_t v = x << 32 | y;
			lua_pushinteger(L, v);
			lua_rawseti(L, -2, i + 1);
		}
	}
	return 1;
}

static int
lbank_ptr(lua_State *L) {
	struct sprite_bank *b = (struct sprite_bank *)luaL_checkudata(L, 1, "SOLUNA_SPRITEBANK");
	lua_pushlightuserdata(L, b);
	return 1;
}

static int
lsprite_newbank(lua_State *L) {
	int cap = luaL_checkinteger(L, 1);
	int texture_size = luaL_optinteger(L, 2, DEFAULT_TEXTURE_SIZE);
	struct sprite_bank *b = (struct sprite_bank *)lua_newuserdatauv(L, sizeof(*b) + (cap-1) * sizeof(b->rect[0]), 0);
	b->n = 0;
	b->cap = cap;
	b->texture_size = texture_size;
	b->texture_n = 0;
	b->current_frame = 0;
	b->texture_ready = 0;
	
	if (luaL_newmetatable(L, "SOLUNA_SPRITEBANK")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "add", lbank_add },
			{ "touch", lbank_touch },
			{ "pack", lbank_pack },
			{ "altas", lbank_altas },
			{ "ptr", lbank_ptr },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	
	return 1;
}

struct batch {
	int n;
	struct draw_batch *b;
};

static int
lbatch_reset(lua_State *L) {
	struct batch *b = (struct batch *)luaL_checkudata(L, 1, "SOLUNA_BATCH");
	b->n = 0;
	return 0;
}

static int
lbatch_add(lua_State *L) {
	struct batch *b = (struct batch *)luaL_checkudata(L, 1, "SOLUNA_BATCH");
	int n = b->n;
	struct draw_primitive * p = batch_reserve(b->b, n + 1);
	if (p == NULL)
		return luaL_error(L, "Out of memory");
	
	p += n;
	
	int id = luaL_checkinteger(L, 2);
	float x = luaL_checknumber(L, 3);
	float y = luaL_checknumber(L, 4);
	
	sprite_set_xy(p, x, y);
	p->sprite = id;
	if (lua_gettop(L) > 4) {
		float scale = luaL_optnumber(L, 5, 1);
		float rot = luaL_optnumber(L, 6, 0);
		sprite_set_sr(p, scale, rot);
	} else {
		p->sr = 0;
	}
	
	b->n = n + 1;
	return 0;
}

static int
lbatch_ptr(lua_State *L) {
	struct batch *b = (struct batch *)luaL_checkudata(L, 1, "SOLUNA_BATCH");
	int offset = luaL_optinteger(L, 2, 0);
	struct draw_primitive * p = batch_reserve(b->b, 0);
	if (offset >= b->n)
		return 0;
	p += offset;
	int n = b->n - offset;
	lua_pushlightuserdata(L, p);
	lua_pushinteger(L, n);
	return 2;
}

static int
lsprite_newbatch(lua_State *L) {
	struct batch *b = (struct batch *)lua_newuserdatauv(L, sizeof(*b), 0);
	b->n = 0;
	b->b = batch_new(0);
	if (b->b == NULL)
		return luaL_error(L, "Out of memory");

	if (luaL_newmetatable(L, "SOLUNA_BATCH")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "reset", lbatch_reset },
			{ "add", lbatch_add },
			{ "ptr", lbatch_ptr },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	
	return 1;
}

int
luaopen_spritemgr(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "newbank", lsprite_newbank },
		{ "newbatch", lsprite_newbatch },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
