#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "sokol/sokol_gfx.h"
#include "sdftext.glsl.h"
#include "srbuffer.h"
#include "batch.h"
#include "spritemgr.h"
#include "font_manager.h"
#include "sprite_submit.h"

#define BATCHN 4096
#define MATERIAL_TEXT_NORMAL 1

struct text {
	int codepoint;
	uint16_t font;
	uint16_t size;
	uint32_t color;
};

struct inst_object {
	float x, y;
	float sr_index;
};

struct buffer_data {
	sprite_t spr[BATCHN];
	struct inst_object inst[BATCHN];
};

struct material_text {
	sg_pipeline pip;
	sg_buffer inst;
	sg_buffer sprite;
	sg_bindings *bind;
	vs_params_t *uniform;
	struct sr_buffer *srbuffer;
	struct font_manager *font;
	fs_params_t fs_uniform;
};

static void
submit(lua_State *L, struct material_text *m, struct draw_primitive *prim, int n) {
	struct buffer_data tmp;
	int i;
	int count = 0;
	for (i=0;i<n;i++) {
		struct draw_primitive *p = &prim[i*2];
		assert(p->sprite == -1);
		
		struct text * t = (struct text *)&prim[i*2+1];
		struct font_glyph g, og;
		const char* err = font_manager_glyph(m->font, t->font, t->codepoint, t->size, &g, &og);
		if (err == NULL) {
			float scale = og.w == 0 ? 0 : (float)g.w / og.w;
			tmp.spr[count].offset = (-og.offset_x + 0x8000) << 16 | (-og.offset_y + 0x8000);
			tmp.spr[count].u = og.u << 16 | FONT_MANAGER_GLYPHSIZE;
			tmp.spr[count].v = og.v << 16 | FONT_MANAGER_GLYPHSIZE;

			sprite_mul_scale(p, scale);
			// calc scale/rot index
			int sr_index = srbuffer_add(m->srbuffer, p->sr);
			if (sr_index < 0) {
				// todo: support multiply srbuffer
				luaL_error(L, "sr buffer is full");
			}
			tmp.inst[count].x = (float)p->x / 256.0f;
			tmp.inst[count].y = (float)p->y / 256.0f;
			tmp.inst[count].sr_index = (float)sr_index;
			++count;
		} else {
			t->codepoint = -1;
		}
	}
	sg_append_buffer(m->inst, &(sg_range) { tmp.inst , count * sizeof(tmp.inst[0]) });
	sg_append_buffer(m->sprite, &(sg_range) { tmp.spr , count * sizeof(tmp.spr[0]) });
}

static int
lmateraial_text_submit(lua_State *L) {
	struct material_text *m = (struct material_text *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_TEXT");
	struct draw_primitive *prim = lua_touserdata(L, 2);
	int prim_n = luaL_checkinteger(L, 3);
	int i;
	for (i=0;i<prim_n;i+=BATCHN) {
		int n = (prim_n - i) % BATCHN;
		submit(L, m, prim, n);
		prim += BATCHN;
	}
	return 0;
}

static void
draw_text(struct material_text *m, uint32_t color, int count) {
	m->fs_uniform.color = color;
	sg_apply_uniforms(UB_vs_params, &(sg_range){ m->uniform, sizeof(vs_params_t) });
	sg_apply_uniforms(UB_fs_params, &(sg_range){ &m->fs_uniform, sizeof(fs_params_t) });
	sg_apply_bindings(m->bind);
	sg_draw(0, 4, count);
	
	m->uniform->baseinst += count;
	m->bind->vertex_buffer_offsets[0] += count * sizeof(struct inst_object);
}

static int
lmateraial_text_draw(lua_State *L) {
	struct material_text *m = (struct material_text *)luaL_checkudata(L, 1, "SOLUNA_MATERIAL_TEXT");
	struct draw_primitive *prim = lua_touserdata(L, 2);
	int prim_n = luaL_checkinteger(L, 3);
	if (prim_n <= 0)
		return 0;
	
	int i;
	float texsize = m->uniform->texsize;
	m->uniform->texsize = 1.0f / FONT_MANAGER_TEXSIZE;
	sg_apply_pipeline(m->pip);
	
	int count = -1;
	uint32_t color = 0;
	for (i=0;i<prim_n;i++) {
		struct text * t = (struct text *)&prim[i*2+1];
		if (t->codepoint >= 0) {
			if (count < 0) {
				color = t->color;
				count = 1;
			} else if (t->color != color) {
				draw_text(m, color, count);
				color = t->color;
				count = 1;
			} else {
				++count;
			}
		}
	}
	draw_text(m, color, count);

	m->uniform->texsize = texsize;

	return 0;
}

static void
ref_object(lua_State *L, void *ptr, int uv_index, const char *key, const char *luatype, int direct) {
	if (lua_getfield(L, 1, key) != LUA_TUSERDATA)
		luaL_error(L, "Invalid key .%s", key);
	void *obj = luaL_checkudata(L, -1, luatype);
	lua_pushvalue(L, -1);
	// ud, object, object
	lua_setiuservalue(L, -3, uv_index);
	if (!direct) {
		lua_pushlightuserdata(L, ptr);
		lua_call(L, 1, 0);
	} else {
		lua_pop(L, 1);
		void **ref = (void **)ptr;
		*ref = obj;
	}
}

static void
init_pipeline(struct material_text *m) {
	sg_shader shd = sg_make_shader(texquad_shader_desc(sg_query_backend()));

	m->pip = sg_make_pipeline(&(sg_pipeline_desc) {
		.layout = {
			.buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
			.attrs = {
					[ATTR_texquad_position].format = SG_VERTEXFORMAT_FLOAT3,
				}
        },
		.colors[0].blend = (sg_blend_state) {
			.enabled = true,
			.src_factor_rgb = SG_BLENDFACTOR_ONE,
			.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.src_factor_alpha = SG_BLENDFACTOR_ONE,
			.dst_factor_alpha = SG_BLENDFACTOR_ZERO
		},
        .shader = shd,
		.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .label = "text-pipeline"
    });
}

static int
lnew_material_text_normal(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct material_text *m = (struct material_text *)lua_newuserdatauv(L, sizeof(*m), 5);
	ref_object(L, &m->inst, 1, "inst_buffer", "SOKOL_BUFFER", 0);
	ref_object(L, &m->sprite, 2, "sprite_buffer", "SOKOL_BUFFER", 0);
	ref_object(L, &m->bind, 3, "bindings", "SOKOL_BINDINGS", 1);
	ref_object(L, &m->uniform, 4, "uniform", "SOKOL_UNIFORM", 1);
	ref_object(L, &m->srbuffer, 5, "sr_buffer", "SOLUNA_SRBUFFER", 1);
	init_pipeline(m);

	if (lua_getfield(L, 1, "font_manager") != LUA_TLIGHTUSERDATA) {
		return luaL_error(L, "Missing .font_manager");
	}
	m->font = lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	m->fs_uniform = (fs_params_t) {
		.edge_mask = font_manager_sdf_mask(m->font),
		.dist_multiplier = 1.0f,
		.color= 0xff000000,
	};

	if (luaL_newmetatable(L, "SOLUNA_MATERIAL_TEXT")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "submit", lmateraial_text_submit },
			{ "draw", lmateraial_text_draw },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

static int
lchar_for_batch(lua_State *L) {
	struct text * t = (struct text *)lua_touserdata(L, lua_upvalueindex(1));
	t->codepoint = luaL_checkinteger(L, 1);
	t->font = luaL_checkinteger(L, 2);
	t->size = luaL_checkinteger(L, 3);
	t->color = luaL_checkinteger(L, 4);
	if (!(t->color & 0xff000000))
		t->color |= 0xff000000;
	lua_pushvalue(L, lua_upvalueindex(1));
	return 1;
}

struct text_primitive {
	struct draw_primitive pos;
	union {
		struct draw_primitive dummy;
		struct text text;
	} u;
};

/*
** From lua utf8lib :
** Decode one UTF-8 sequence, returning NULL if byte sequence is
** invalid.  The array 'limits' stores the minimum value for each
** sequence length, to check for overlong representations. Its first
** entry forces an error for non-ascii bytes with no continuation
** bytes (count == 0).
*/

#define iscont(c)	(((c) & 0xC0) == 0x80)
#define l_uint32 uint32_t
#define MAXUTF		0x7FFFFFFFu

static const char *utf8_decode (const char *s, l_uint32 *val) {
  static const l_uint32 limits[] =
        {~(l_uint32)0, 0x80, 0x800, 0x10000u, 0x200000u, 0x4000000u};
  unsigned int c = (unsigned char)s[0];
  l_uint32 res = 0;  /* final result */
  if (c < 0x80)  /* ascii? */
    res = c;
  else {
    int count = 0;  /* to count number of continuation bytes */
    for (; c & 0x40; c <<= 1) {  /* while it needs continuation bytes... */
      unsigned int cc = (unsigned char)s[++count];  /* read next byte */
      if (!iscont(cc))  /* not a continuation byte? */
        return NULL;  /* invalid byte sequence */
      res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
    }
    res |= ((l_uint32)(c & 0x7F) << (count * 5));  /* add first byte */
    if (count > 5 || res > MAXUTF || res < limits[count])
      return NULL;  /* invalid byte sequence */
    s += count;  /* skip continuation bytes read */
  }
  *val = res;
  return s + 1;  /* +1 to include first byte */
}

static const char *
skip_bracket(const char *str) {
	for (;;) {
		if (*str == ']') {
			return str + 1;
		} else if (*str == '\0') {
			return str;
		}
		++str;
	}
}

static int
count_string(const char *str) {
	uint32_t val = 0;
	int n = 0;
	while ((str = utf8_decode(str, &val))) {
		if (val == 0)
			break;
		if (val > 32) {
			if (val == '[') {
				char c = *str;
				if (c == '[') {
					++str;
					++n;
				} else {
					if (c == 'i') {
						// icons
						++n;
					}
					str = skip_bracket(str);
				}
			} else {
				++n;
			}
		}
	}
	return n;
}

#define MAX_WIDTH 4096
#define MAX_HEIGHT 4096
#define DEFAULT_FONTSIZE 24

static void *
free_primitive(void *ud, void *ptr, size_t osize, size_t nsize) {
	free(ptr);
	return NULL;
}

// todo: support multi font/size
struct block_context {
	int width;
	int height;
	int x;
	int y;
	int ascent;
	int decent;
	uint32_t default_color;
	uint32_t color;
};

static inline int
advance(struct block_context *ctx, int x) {
	if (x + ctx->x > ctx->width)
		return 1;
	ctx->x += x;
	return 0;
}

static inline int
newline(struct block_context *ctx) {
	if (ctx->y + ctx->ascent + ctx->decent > ctx->height)
		return 1;
	ctx->y += ctx->ascent + ctx->decent;
	ctx->x = 0;
	return 0;
}

static inline int
tohex(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static const char *
parse_bracket(struct block_context *ctx, const char *str, int *icon) {
	char c = *str;
	int hex = -1;
	if (c == 'i') {
		++str;
		int num = 0;
		while (*str >= '0' && *str <= '9') {
			num = num * 10 + (*str - '0');
			++str;
		}
		*icon = num + 1;
	} else if ((hex = tohex(c)) >= 0) {
		int color = hex;
		for (;;) {
			++str;
			if ((hex = tohex(*str)) >= 0) {
				color = color * 16 + hex;
			} else {
				break;
			}
		}
		if (!(color & 0xff000000))
			color |= 0xff000000;
		ctx->color = color;
	} else if (c == 'n') {
		ctx->color = ctx->default_color;
	}
	// todo: other command
	return skip_bracket(str);
}

#define ALIGNMENT_LEFT 0
#define ALIGNMENT_CENTER 1
#define ALIGNMENT_RIGHT 2
#define ALIGNMENT_MASK 3
#define VALIGNMENT_TOP (1<<2)
#define VALIGNMENT_CENTER 0
#define VALIGNMENT_BOTTOM (2<<2)
#define VALIGNMENT_MASK (3<<2)

// todo: support color
static int
ltext(lua_State *L) {
	const char * str = luaL_checkstring(L, 1);
	int count = count_string(str);
	struct block_context ctx;
	ctx.width = luaL_optinteger(L, 2, MAX_WIDTH);
	ctx.height = luaL_optinteger(L, 3, MAX_HEIGHT);
	struct font_manager *mgr = (struct font_manager *)lua_touserdata(L, lua_upvalueindex(1));
	int fontid = lua_tointeger(L, lua_upvalueindex(2));
	int fontsize = lua_tointeger(L, lua_upvalueindex(3));
	ctx.default_color = lua_tointeger(L, lua_upvalueindex(4));
	ctx.color = ctx.default_color;
	ctx.x = 0;
	int decent, gap;
	font_manager_fontheight(mgr, fontid, fontsize, &ctx.ascent, &decent, &gap);
	ctx.decent = -decent + gap;
	ctx.y = ctx.ascent;
	
	char * buffer = (char *)malloc(count * sizeof(struct text_primitive)+1);
	struct text_primitive * prim = (struct text_primitive *)buffer;
	int i;
	int n = 0;
	int width = 0;
	for (i=0;i<count;) {
		uint32_t val = 0;
		str = utf8_decode(str, &val);
		if (val <= 32) {
			if (val == '\n') {
				if (ctx.x > width)
					width = ctx.x;
				if (newline(&ctx))
					break;
			} else {
				struct font_glyph g, og;
				if (font_manager_glyph(mgr, fontid, 32, fontsize, &g, &og) == NULL) {
					if (ctx.x > width)
						width = ctx.x;
					if (advance(&ctx, g.advance_x)) {
						if (newline(&ctx))
							break;
						advance(&ctx, g.advance_x);
					}
				}
			}
		} else {
			int icon = 0;
			if (val == '[') {
				if (*str != '[') {
					str = parse_bracket(&ctx, str, &icon);
					if (!icon) {
						continue;
					}
				} else {
					++str;
				}
			}
			prim[n].pos.x = ctx.x * 256;
			prim[n].pos.y = ctx.y * 256;
			prim[n].pos.sr = 0;
			prim[n].pos.sprite = -MATERIAL_TEXT_NORMAL;
			
			int codepoint = val;
			int font = fontid;
			
			if (icon > 0) {
				codepoint = icon -1;
				font = FONT_ICON;
				prim[n].pos.y -= ( ctx.ascent ) * 256;
			}
			
			struct font_glyph g, og;
			if (font_manager_glyph(mgr, font, codepoint, fontsize, &g, &og) == NULL) {
				if (ctx.x > width)
					width = ctx.x;
				if (advance(&ctx, g.advance_x)) {
					if (newline(&ctx))
						break;
					prim[n].pos.x = ctx.x * 256;
					prim[n].pos.y = ctx.y * 256;
					advance(&ctx, g.advance_x);
				}
				prim[n].u.text.codepoint = codepoint;
				prim[n].u.text.font = font;
				prim[n].u.text.size = fontsize;
				prim[n].u.text.color = ctx.color;
				++n;
			}
			++i;
		}
	}
	if (ctx.x > width)
		width = ctx.x;
	int height = ctx.y + ctx.decent;
	int alignment = lua_tointeger(L, lua_upvalueindex(5));
	int offx, offy;
	int align = alignment & ALIGNMENT_MASK;
	switch (align) {
	case ALIGNMENT_CENTER:
		offx = (ctx.width - width) / 2 * 256;
		break;
	case ALIGNMENT_RIGHT:
		offx = (ctx.width - width) * 256;
		break;
	default:
		offx = 0;
		break;
	}
	int valign = alignment & VALIGNMENT_MASK;
	switch (valign) {
	case VALIGNMENT_CENTER:
		offy = (ctx.height - height) / 2 * 256;
		break;
	case VALIGNMENT_BOTTOM:
		offy = (ctx.height - height) * 256;
		break;
	default:
		offy = 0;
		break;
	}
	if (offx != 0 || offy != 0) {
		for (i=0;i<n;i++) {
			prim[i].pos.x += offx;
			prim[i].pos.y += offy;
		}
	}
	lua_pushexternalstring(L, buffer, n * sizeof(struct text_primitive), free_primitive, NULL);
	return 1;
}

static uint32_t
parse_alignment(lua_State *L, int index) {
	const char *alignment_string = lua_tostring(L, index);
	int i;
	char c;
	uint32_t alignment = 0;
	for (i=0;(c = alignment_string[i]);i++) {
		switch(c) {
		case 'l' :
		case 'L' :
			alignment |= ALIGNMENT_LEFT;
			break;
		case 'r' :
		case 'R' :
			alignment |= ALIGNMENT_RIGHT;
			break;
		case 'c' :
		case 'C' :
			alignment |= ALIGNMENT_CENTER;
			break;
		case 't' :
		case 'T' :
			alignment |= VALIGNMENT_TOP;
			break;
		case 'v' :
		case 'V' :
			alignment |= VALIGNMENT_CENTER;
			break;
		case 'b' :
		case 'B' :
			alignment |= VALIGNMENT_BOTTOM;
			break;
		}
	}
	return alignment;
}

static int
ltext_block(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	void * font_mgr = lua_touserdata(L, 1);
	int fontid = luaL_checkinteger(L, 2);
	int fontsize = luaL_optinteger(L, 3, DEFAULT_FONTSIZE);
	uint32_t color = luaL_optinteger(L, 4, 0xff000000);
	uint32_t alignment = 0;
	if (lua_type(L, 5) == LUA_TSTRING) {
		alignment = parse_alignment(L, 5);
	}
	if (!(color & 0xff000000))
		color |= 0xff000000;
	lua_pushlightuserdata(L, font_mgr);	// 1
	lua_pushinteger(L, fontid);	// 2
	lua_pushinteger(L, fontsize);	// 3
	lua_pushinteger(L, color);	// 4
	lua_pushinteger(L, alignment);	// 5
	lua_pushcclosure(L, ltext, 5);
	return 1;
}

int
luaopen_material_text(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "char", NULL },
		{ "block", ltext_block },
		{ "normal", lnew_material_text_normal },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	
	// char()
	struct text * t = lua_newuserdatauv(L, sizeof(*t), 1);
	memset(t, 0, sizeof(*t));
	lua_pushinteger(L, MATERIAL_TEXT_NORMAL);
	lua_setiuservalue(L, -2, 1);
	lua_pushcclosure(L, lchar_for_batch, 1);
	lua_setfield(L, -2, "char");
	
	return 1;
}
