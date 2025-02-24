#include <lua.h>
#include <lauxlib.h>

#include "draw.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sprite_submit.h"
#include "texquad.glsl.h"

#include <stdint.h>
#include <string.h>
#include <assert.h>

struct vertex_t {
	uint32_t offset;
	uint32_t u;
	uint32_t v;
};

#define MAX_SPRITE_VERTEX (64 * 1024 / sizeof(struct vertex_t))

struct instance_t {
	float x, y;
	float sr;
};

static void
draw_state_init(struct draw_state *state, int image_id, int inst_id, int sr_id, int sprite_id) {
	state->bind.vertex_buffers[0].id = inst_id;
	state->bind.storage_buffers[SBUF_sr_lut].id = sr_id;
	state->bind.storage_buffers[SBUF_sprite_buffer].id = sprite_id;

	// create a default sampler object with default attributes
    state->bind.samplers[SMP_smp] = sg_make_sampler(&(sg_sampler_desc){
        .label = "texquad-sampler"
    });
	
	state->bind.images[IMG_tex].id = image_id;

	srbuffer_init(&state->srb_mem);
}

static inline uint32_t
pack_short2(int16_t a, int16_t b) {
	uint32_t v1 = a + 0x8000;
	uint32_t v2 = b + 0x8000;
	return v1 << 16 | v2;
}

static inline uint32_t
pack_ushort2(uint16_t a, uint16_t b) {
	return a << 16 | b;
}

struct sprite_rect {
	int dx, dy;
	int uv[4];
};

static void
draw_state_commit(struct draw_state *state, struct sprite_rect *rect) {
	srbuffer_init(&state->srb_mem);
	struct draw_primitive tmp;
	uint64_t count = sapp_frame_count();
	float rad = count * 3.1415927f / 180.0f;
	float s = sinf(rad) + 1.2f;
	sprite_set_sr(&tmp, s, rad);
	int index1 = srbuffer_add(&state->srb_mem, tmp.sr);
	sprite_set_sr(&tmp, s, -rad);
	int index2 = srbuffer_add(&state->srb_mem, tmp.sr);
	assert(index1 <= 3);
	assert(index2 <= 3);
	int x = 256, y = 256;
	struct vertex_t vertices[] = {
		{ 
			pack_short2(rect->dx, rect->dy),
			pack_ushort2(rect->uv[0], rect->uv[1]),
			pack_ushort2(rect->uv[2], rect->uv[3]),
		},
//		{ pack_short2(0, 0), pack_ushort2(0, 128), pack_ushort2(0, 128) },
	};
	struct instance_t inst[] = {
		{ x, y, (float)index1 },
//		{ x+100, y+100, (float)index2 },
	};
	sg_update_buffer(state->bind.vertex_buffers[0], &SG_RANGE(inst));
	sg_update_buffer(state->bind.storage_buffers[SBUF_sr_lut], &(sg_range){ state->srb_mem.data, state->srb_mem.n * sizeof(struct sr_mat)});
	sg_update_buffer(state->bind.storage_buffers[SBUF_sprite_buffer], &SG_RANGE(vertices));

	sg_apply_bindings(&state->bind);
	sg_draw(0, 4, sizeof(inst)/sizeof(inst[0]));
}

static int
render_init(lua_State *L) {
	struct draw_state * S = (struct draw_state *)lua_newuserdatauv(L, sizeof(*S), 0);
	int image_id = luaL_checkinteger(L, 1);
	int inst_id = luaL_checkinteger(L, 2);
	int sr_id = luaL_checkinteger(L, 3);
	int sprite_id = luaL_checkinteger(L, 4);
	draw_state_init(S, image_id, inst_id, sr_id, sprite_id);
	return 1;
}

static int
render_commit(lua_State *L) {
	struct draw_state *S = lua_touserdata(L, 1);
	struct sprite_rect rect;
	rect.dx = luaL_checkinteger(L, 2);
	rect.dy = luaL_checkinteger(L, 3);
	rect.uv[0] = luaL_checkinteger(L, 4);
	rect.uv[1] = rect.uv[0] +luaL_checkinteger(L, 6);
	rect.uv[2] = luaL_checkinteger(L, 5);
	rect.uv[3] = rect.uv[2] + luaL_checkinteger(L, 7);
	draw_state_commit(S, &rect);
	return 0;
}

int
luaopen_draw(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "init", render_init },
		{ "commit", render_commit },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
