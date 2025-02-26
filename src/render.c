#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdint.h>

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_app.h"
#include "texquad.glsl.h"
#include "srbuffer.h"
#include "sprite_submit.h"
#include "batch.h"
#include "spritemgr.h"

#define UNIFORM_MAX 4
#define BINDINGNAME_MAX 32

struct buffer {
	sg_buffer handle;
	int type;
};

struct image {
	sg_image img;
	int width;
	int height;
};

struct sampler {
	sg_sampler handle;
};

static int
get_buffer_type(lua_State *L, int index) {
	if (lua_getfield(L, index, "type") != LUA_TSTRING) {
		return luaL_error(L, "Need .type");
	}
	const char * str = lua_tostring(L, -1);
	int t = 0;
	if (strcmp(str, "vertex") == 0) {
		t = SG_BUFFERTYPE_VERTEXBUFFER;
	} else if (strcmp(str, "index") == 0) {
		t = SG_BUFFERTYPE_INDEXBUFFER;
	} else if (strcmp(str, "storage") == 0) {
		t = SG_BUFFERTYPE_STORAGEBUFFER;
	} else {
		return luaL_error(L, "Invalid buffer .type = %s", str);
	}
	lua_pop(L, 1);
	return t;
}

static int
get_buffer_usage(lua_State *L, int index) {
	if (lua_getfield(L, index, "usage") != LUA_TSTRING) {
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			return SG_USAGE_IMMUTABLE;
		}
		return luaL_error(L, "Invalid .usage");
	}
	const char * str = lua_tostring(L, -1);
	int t = 0;
	if (strcmp(str, "stream") == 0) {
		t = SG_USAGE_STREAM;
	} else if (strcmp(str, "dynamic") == 0) {
		t = SG_USAGE_DYNAMIC;
	} else if (strcmp(str, "immutable") == 0) {
		t = SG_USAGE_IMMUTABLE;
	} else {
		return luaL_error(L, "Invalid buffer .usage = %s", str);
	}
	lua_pop(L, 1);
	return t;
}

static const void *
get_buffer_data(lua_State *L, int index, size_t *sz) {
	int t = lua_getfield(L, index, "data");
	if (t == LUA_TNIL) {
		// no ptr
		lua_pop(L, 1);
		if (lua_getfield(L, index, "size") != LUA_TNUMBER) {
			luaL_error(L, "No .data and .size");
		}
		*sz = luaL_checkinteger(L, -1);
		lua_pop(L, 1);
		return NULL;
	}
	size_t size = 0;
	if (lua_getfield(L, index, "size") == LUA_TNUMBER) {
		size = luaL_checkinteger(L, -1);
	}
	lua_pop(L, 1);
	if (t == LUA_TLIGHTUSERDATA) {
		if (size == 0) {
			luaL_error(L, "lightuserdata for .data without .size");
		}
		*sz = size;
		const void * ptr = lua_touserdata(L, -1);
		lua_pop(L, 1);
		return ptr;
	}
	else if (t == LUA_TUSERDATA) {
		size_t rawlen = lua_rawlen(L, -1);
		if (size > 0 && size != rawlen)
			luaL_error(L, "size of userdata %d != %d", rawlen, size);
		const void * ptr = lua_touserdata(L, -1);
		lua_pop(L, 1);
		*sz = size;
		return ptr;
	} else if (t == LUA_TSTRING) {
		size_t rawlen;
		const void * ptr = (const void *)lua_tolstring(L, -1, &rawlen);
		if (size > 0 && size != rawlen)
			luaL_error(L, "size of string %d != %d", rawlen, size);
		lua_pop(L, 1);
		*sz = size;
		return ptr;
	}
	luaL_error(L, "Invalid .data type = %s", lua_typename(L, t));
	*sz = 0;
	return NULL;
}

static int
lbuffer_update(lua_State *L) {
	if (lua_gettop(L) == 1)
		return 0;
	struct buffer *p = (struct buffer *)luaL_checkudata(L, 1, "SOKOL_BUFFER");
	size_t sz;
	const void *ptr;
	switch (lua_type(L, 2)) {
	case LUA_TSTRING:
		ptr = (const void *)lua_tolstring(L, 2, &sz);
		break;
	case LUA_TUSERDATA:
		ptr = (const void *)lua_touserdata(L, 2);
		sz = lua_rawlen(L, 2);
		if (lua_isinteger(L, 3)) {
			int usersize = lua_tointeger(L, 3);
			if (usersize > sz)
				return luaL_error(L, "Invalid size %d > %d", usersize, sz);
			sz = usersize;
		}
		break;
	case LUA_TLIGHTUSERDATA:
		ptr = (const void *)lua_touserdata(L, 2);
		sz = luaL_checkinteger(L, 3);
		break;
	default:
		return luaL_error(L, "Invalid data type %s", lua_typename(L, lua_type(L, 2)));
	}
	sg_update_buffer(p->handle, &(sg_range) { ptr, sz });
	return 0;
}

static int
lbuffer(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct buffer * p = (struct buffer *)lua_newuserdatauv(L, sizeof(*p), 0);
	p->type = get_buffer_type(L, 1);
	int usage = get_buffer_usage(L, 1);
	size_t sz;
	const void *ptr = get_buffer_data(L, 1, &sz);
	if (usage == SG_USAGE_IMMUTABLE && ptr == NULL) {
		return luaL_error(L, "immutable buffer needs init data");
	}
	const char *label = NULL;
	if (lua_getfield(L, 1, "label") == LUA_TSTRING) {
		label = lua_tostring(L, -1);
	}
	lua_pop(L, 1);
	p->handle = sg_make_buffer(&(sg_buffer_desc) {
		.size = sz,
		.type = p->type,
		.usage = usage,
		.label = label,
	    .data.ptr = ptr,
		.data.size = sz,
	});
		
	if (luaL_newmetatable(L, "SOKOL_BUFFER")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "update", lbuffer_update },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);

	return 1;
}

// todo : offscreen pass
struct pass {
	sg_pass_action pass_action;
};

static int
read_color_action(lua_State *L, int index, sg_pass_action *action, int idx) {
	char key[] = { 'c', 'o', 'l', 'o' , 'r' , '0' + idx, '\0' };
	int t = lua_getfield(L, index, key);
	if (t == LUA_TNIL) {
		lua_pop(L, 1);
		return 0;
	}
	if (idx >= SG_MAX_COLOR_ATTACHMENTS)
		return luaL_error(L, "Too many color attachments %d >= %d", idx , SG_MAX_COLOR_ATTACHMENTS);
	if ( t == LUA_TSTRING ) {
		const char * key = lua_tostring(L, -1);
		if (strcmp(key, "load") == 0) {
			action->colors[idx].load_action = SG_LOADACTION_LOAD;
		} else if (strcmp(key, "dontcare") == 0 ) {
			action->colors[idx].load_action = SG_LOADACTION_DONTCARE;
		} else {
			return luaL_error(L, "Invalid load action (%d) = %s", idx, key);
		}
	} else {
		uint32_t c = luaL_checkinteger(L, -1);
		if (c <= 0xffffff) {
			action->colors[idx].clear_value.a = 1.0f;
		} else {
			action->colors[idx].clear_value.a = ((c & 0xff000000) >> 24) / 255.0f;
		}
		action->colors[idx].clear_value.r = ((c & 0xff0000) >> 16) / 255.0f;
		action->colors[idx].clear_value.g = ((c & 0x00ff00) >> 8) / 255.0f;
		action->colors[idx].clear_value.b = ((c & 0x0000ff)) / 255.0f;
		action->colors[idx].load_action = SG_LOADACTION_CLEAR;
	}
	lua_pop(L, 1);
	return 1;
}

static int
lpass_begin(lua_State *L) {
	struct pass * p = (struct pass *)luaL_checkudata(L, 1, "SOKOL_PASS");
	sg_begin_pass(&(sg_pass) { .action = p->pass_action, .swapchain = sglue_swapchain() });
	return 0;
}

static int
lpass_end(lua_State *L) {
	sg_end_pass();
	return 0;
}

static int
lpass_new(lua_State *L) {
	struct pass * p = lua_newuserdatauv(L, sizeof(*p), 0);
	memset(p, 0, sizeof(*p));
	luaL_checktype(L, 1, LUA_TTABLE);
	sg_pass_action *action = &p->pass_action;
	// todo : store action
	
	int i = 0;
	while (read_color_action(L, 1, action, i)) {
		++i;
	}
	if (lua_getfield(L, 1, "depth") == LUA_TNIL) {
		action->depth.load_action = SG_LOADACTION_DONTCARE;
	} else {
		float depth = luaL_checknumber(L, -1);
		action->depth.load_action = SG_LOADACTION_CLEAR;
		action->depth.clear_value = depth;
	}
	lua_pop(L, 1);
	if (lua_getfield(L, 1, "stencil") == LUA_TNIL) {
		action->depth.load_action = SG_LOADACTION_DONTCARE;
	} else {
		int s = luaL_checkinteger(L, -1);
		if (s < 0 || s > 255)
			return luaL_error(L, "Invalid stencil %d", s);
		action->depth.load_action = SG_LOADACTION_CLEAR;
		action->depth.clear_value = s;
	}
	lua_pop(L, 1);
	if (luaL_newmetatable(L, "SOKOL_PASS")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "begin", lpass_begin },
			{ "finish", lpass_end },	// end is a reserved keyword in lua, use finish instead
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
lsubmit(lua_State *L) {
	sg_commit();
	return 0;
}

struct uniform_desc {
	int slot;
	int size;
};

struct bindings_desc {
	int vertexbuffer;
	int indexbuffer;
	char storagebuffer_name[SG_MAX_STORAGEBUFFER_BINDSLOTS][BINDINGNAME_MAX];
	char sampler_name[SG_MAX_SAMPLER_BINDSLOTS][BINDINGNAME_MAX];
	char image_name[SG_MAX_IMAGE_BINDSLOTS][BINDINGNAME_MAX];
};

struct pipeline {
	sg_pipeline pip;
	struct uniform_desc uniform[UNIFORM_MAX];
	struct bindings_desc bindings;
};

static void
default_pipeline(struct pipeline *p) {
	sg_shader shd = sg_make_shader(texquad_shader_desc(sg_query_backend()));

	p->pip = sg_make_pipeline(&(sg_pipeline_desc) {
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
        .label = "default-pipeline"
    });
	p->uniform[0] = (struct uniform_desc){ UB_vs_params , sizeof(vs_params_t) };
	p->bindings.vertexbuffer = 1;
	p->bindings.indexbuffer = 0;
	strcpy(p->bindings.storagebuffer_name[SBUF_sr_lut], "sr_lut");
	strcpy(p->bindings.storagebuffer_name[SBUF_sprite_buffer], "sprite_buffer");
	strcpy(p->bindings.sampler_name[SMP_smp], "smp");
	strcpy(p->bindings.image_name[IMG_tex], "tex");
}

static int
lpipeline_apply(lua_State *L) {
	struct pipeline * p = (struct pipeline *)luaL_checkudata(L, 1, "SOKOL_PIPELINE");
	sg_apply_pipeline(p->pip);
	return 0;
}

struct uniform {
	int slot;
	int size;
	uint8_t buffer[1];
};

#define UNIFORM_TYPE_FLOAT 1

static int
luniform_apply(lua_State *L) {
	struct uniform * p = (struct uniform *)luaL_checkudata(L, 1, "SOKOL_UNIFORM");
	sg_apply_uniforms(UB_vs_params, &(sg_range){ p->buffer, p->size });
	return 0;
}

#define MAX_ARRAY_SIZE 0x4fff

static void
set_uniform_float(lua_State *L, int index, struct uniform *p, int offset, int n) {
	if (n <= 1) {
		float v = luaL_checknumber(L, index);
		memcpy(p->buffer + offset, &v, sizeof(float));
	} else {
		luaL_checktype(L, index, LUA_TTABLE);
		if (lua_rawlen(L, index) != n || n >= MAX_ARRAY_SIZE) {
			luaL_error(L, "Need table size %d", n);
		}
		float v[MAX_ARRAY_SIZE];
		int i;
		for (i=0;i<n;i++) {
			if (lua_rawgeti(L, index, i+1) != LUA_TNUMBER) {
				luaL_error(L, "Invalid value in table[%d]", i+1);
			}
			v[i] = lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
		memcpy(p->buffer + offset, &v, sizeof(float) * n);
	}
}

static int
luniform_set(lua_State *L) {
	struct uniform * p = (struct uniform *)lua_touserdata(L, 1);
	if (p == NULL || lua_getiuservalue(L, 1, 1) != LUA_TTABLE)
		return luaL_error(L, "Invalid uniform setter, call init first");
	lua_pushvalue(L, 2);	// key
	if (lua_rawget(L, -2) != LUA_TNUMBER)
		return luaL_error(L, "No uniform %s", lua_tostring(L, 2));
	// ud key value desc meta
	uint32_t meta = lua_tointeger(L, -1);
	lua_pop(L, 2);
	int type = meta >> 28;
	int offset = (meta >> 14) & 0x3fff;
	int n = meta & 0x3fff;
	switch (type) {
	case UNIFORM_TYPE_FLOAT:
		set_uniform_float(L, 3, p, offset, n);
		break;
	default:
		return luaL_error(L, "Invalid uniform setter typeid %s (%d)", lua_tostring(L, 2), type);
	}
	return 0;
}

static void
set_key(lua_State *L, int index, struct uniform *u) {
	const char * key = lua_tostring(L, -2);
	if (lua_getfield(L, -1, "offset") != LUA_TNUMBER) {
		luaL_error(L, "Missing .%s .offset", key);
	}
	int offset = luaL_checkinteger(L, -1);
	lua_pop(L, 1);
	if (offset < 0 || offset > 0x3fff)
		luaL_error(L, "Invalid .%s offset = %d", key, offset);
	
	int t = lua_getfield(L, -1, "n");
	int n = 1;
	if (t != LUA_TNUMBER && t != LUA_TNIL) {
		luaL_error(L, "Invalid type .%s .n (%s)", key, lua_typename(L, t));
	}
	if (t == LUA_TNUMBER) {
		n = luaL_checkinteger(L, -1);
	}
	if (n < 1 || n > 0x3fff)
		luaL_error(L, "Invalid .%s n = %d", key, n);
	lua_pop(L, 1);
	if (lua_getfield(L, -1, "type") != LUA_TSTRING) {
		luaL_error(L, "Missing .%s .type", key);
	}
	const char *type = lua_tostring(L, -1);
	int tid = 0;
	int typesize = 0;
	if (strcmp(type, "float") == 0) {
		tid = UNIFORM_TYPE_FLOAT;
		typesize = sizeof(float);
	} else {
		luaL_error(L, "Invalid .%s .type %s", key, type);
	}
	lua_pop(L, 1);
	int offset_end = offset + n * typesize;
	if (offset_end > u->size)
		luaL_error(L, "Invalid uniform .%s (offset %d,n %d)", key, offset, n);
	int i;
	for (i=offset;i<offset_end;i++) {
		if (u->buffer[i] != 0)
			luaL_error(L, "Overlap uniform .%s (offset %d,n %d)", key, offset, n);
		u->buffer[i] = 1;
	}
	uint32_t info = (tid << 28) | (offset << 14) | n;
	lua_pushvalue(L, -2);
	lua_pushinteger(L, info);
	lua_rawset(L, index);
}

static void
check_uniform(lua_State *L, struct uniform *u) {
	int i;
	for (i=0;i<u->size;i++) {
		if (u->buffer[i] == 0)
			luaL_error(L, "Uniform is not complete");
	}
}

static int
luniform_init(lua_State *L) {
	struct uniform * p = (struct uniform *)luaL_checkudata(L, 1, "SOKOL_UNIFORM");
	luaL_checktype(L, 2, LUA_TTABLE);
	memset(p->buffer, 0, p->size);
	lua_newtable(L);	// typeinfo for uniform
	int typeinfo = lua_gettop(L);
	
	// iter desc table 2
	lua_pushnil(L);
	while (lua_next(L, 2) != 0) {
		if (lua_type(L, -2) != LUA_TSTRING) {
			return luaL_error(L, "Init error : none string key");
		}
		if (lua_type(L, -1) != LUA_TTABLE) {
			return luaL_error(L, "Init error : invalid .%s", lua_tostring(L, -2));
		}
		set_key(L, typeinfo, p);
		lua_pop(L, 1);
	}
	check_uniform(L, p);
	memset(p->buffer, 0, p->size);
	lua_settop(L, typeinfo);
	lua_setiuservalue(L, 1, 1);
	lua_settop(L, 1);
	return 1;
}

static int
create_uniform(lua_State *L, struct uniform_desc *desc) {
	size_t sz = desc->size + sizeof(struct uniform) - 1;
	struct uniform * u = (struct uniform *)lua_newuserdatauv(L, sz, 1);
	u->slot = desc->slot;
	u->size = desc->size;
	if (luaL_newmetatable(L, "SOKOL_UNIFORM")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "__newindex", luniform_set },
			{ "apply", luniform_apply },
			{ "init", luniform_init },
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
lpipe_uniform_slot(lua_State *L) {
	struct pipeline * p = (struct pipeline *)luaL_checkudata(L, 1, "SOKOL_PIPELINE");
	int idx = luaL_optinteger(L, 2, 0);
	if (idx < 0 || idx >= UNIFORM_MAX)
		return luaL_error(L, "Invalid uniform slot index %d", idx);
	struct uniform_desc * desc = &p->uniform[idx];
	if (desc->size == 0)
		return luaL_error(L, "Undefined uniform slot %d", idx);
	
	create_uniform(L, desc);
	
	return 1;
}

struct bindings {
	sg_bindings bind;
	struct bindings_desc *desc;
};

static inline const char *
cmphead_(const char *key, const char *name, size_t sz) {
	if (memcmp(key, name, sz))
		return NULL;
	return key+sz;
}

#define cmphead(key, name) cmphead_(key, name, sizeof(name "")-1)

static sg_buffer
check_buffer(lua_State *L, int index, int type) {
	struct buffer *p = (struct buffer *)luaL_checkudata(L, index, "SOKOL_BUFFER");
	if (p->type != type) {
		luaL_error(L, "Invalid buffer type %d != %d", type, p->type);
	}
	return p->handle;
}

static sg_image
check_image(lua_State *L, int index) {
	struct image *p = (struct image *)luaL_checkudata(L, index, "SOKOL_IMAGE");
	return p->img;
}

static sg_sampler
check_sampler(lua_State *L, int index) {
	struct sampler *p = (struct sampler *)lua_touserdata(L, index);
	if (p == NULL)
		luaL_error(L, "Invalid sampler");
	return p->handle;
}

static int
lbindings_set(lua_State *L) {
	struct bindings * b = (struct bindings *)luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	const char * key = luaL_checkstring(L, 2);
	const char * tail;
	if ((tail=cmphead(key, "vbuffer"))) {
		int n = *tail - '0';
		if (n >= 0 || n < b->desc->vertexbuffer) {
			b->bind.vertex_buffers[n] = check_buffer(L, 3, SG_BUFFERTYPE_VERTEXBUFFER);
			return 0;
		}
	} else if ((tail=cmphead(key, "ibuffer"))) {
		if (*tail == 0 && b->desc->indexbuffer) {
			b->bind.index_buffer = check_buffer(L, 3, SG_BUFFERTYPE_INDEXBUFFER);
			return 0;
		}
	} else if ((tail=cmphead(key, "sbuffer_"))) {
		int i;
		for (i=0;i<SG_MAX_STORAGEBUFFER_BINDSLOTS;i++) {
			if (strcmp(tail, b->desc->storagebuffer_name[i])==0) {
				b->bind.storage_buffers[i] = check_buffer(L, 3, SG_BUFFERTYPE_STORAGEBUFFER);
				return 0;
			}
		}
	} else if ((tail=cmphead(key, "image_"))) {
		int i;
		for (i=0;i<SG_MAX_IMAGE_BINDSLOTS;i++) {
			if (strcmp(tail, b->desc->image_name[i])==0) {
				b->bind.images[i] = check_image(L, 3);
				return 0;
			}
		}
	} else if ((tail=cmphead(key, "sampler_"))) {
		int i;
		for (i=0;i<SG_MAX_SAMPLER_BINDSLOTS;i++) {
			if (strcmp(tail, b->desc->sampler_name[i])==0) {
				b->bind.samplers[i] = check_sampler(L, 3);
				return 0;
			}
		}
	}

	return luaL_error(L, "Invalid key .%s", key);
}

static int
lbindings_apply(lua_State *L) {
	struct bindings * b = (struct bindings *)luaL_checkudata(L, 1, "SOKOL_BINDINGS");
	sg_apply_bindings(&b->bind);
	return 0;
}

static int
lpipeline_bindings(lua_State *L) {
	struct pipeline * p = (struct pipeline *)luaL_checkudata(L, 1, "SOKOL_PIPELINE");
	struct bindings * b = (struct bindings *)lua_newuserdatauv(L, sizeof(*b), 1);
	lua_pushvalue(L, 1);
	lua_setiuservalue(L, -2, 1);
	memset(&b->bind, 0, sizeof(b->bind));
	b->desc = &p->bindings;
	if (luaL_newmetatable(L, "SOKOL_BINDINGS")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "__newindex", lbindings_set },
			{ "apply", lbindings_apply },
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
lpipeline(lua_State *L) {
	const char * name = luaL_checkstring(L, 1);
	struct pipeline * pip = (struct pipeline *)lua_newuserdatauv(L, sizeof(*pip), 0);
	memset(pip, 0, sizeof(*pip));
	if (strcmp(name, "default") == 0) {
		default_pipeline(pip);
	} else {
		// todo : externl shaders and pipelines
		return luaL_error(L, "Invalid pipeline name");
	}
	if (luaL_newmetatable(L, "SOKOL_PIPELINE")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "apply", lpipeline_apply },
			{ "uniform_slot", lpipe_uniform_slot },
			{ "bindings", lpipeline_bindings },
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
limage_update(lua_State *L) {
	struct image *p = (struct image *)luaL_checkudata(L, 1, "SOKOL_IMAGE");
	// todo: support subimage
	luaL_checktype(L, 2, LUA_TUSERDATA);
	void *buffer = lua_touserdata(L, 2);
	sg_image_data data = {
		.subimage[0][0].ptr = buffer,
		.subimage[0][0].size = p->width * p->height * 4,
	};
	sg_update_image(p->img, &data);
	return 0;
}

static int
limage(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	sg_image_desc img = { .usage = SG_USAGE_DYNAMIC };
	if (lua_getfield(L, 1, "width") != LUA_TNUMBER) {
		return luaL_error(L, "Need .width");
	}
	img.width = luaL_checkinteger(L, -1);
	lua_pop(L, 1);
	if (lua_getfield(L, 1, "height") != LUA_TNUMBER) {
		return luaL_error(L, "Need .height");
	}
	img.height = luaL_checkinteger(L, -1);
	lua_pop(L, 1);
	if (lua_getfield(L, 1, "label") == LUA_TSTRING) {
		img.label = lua_tostring(L, -1);
	}
	lua_pop(L, 1);
	// todo: type, render_target, num_slices, num_mipmaps, pixel_format, etc
	struct image * p = (struct image *)lua_newuserdatauv(L, sizeof(*p), 0);
	memset(p, 0, sizeof(*p));
	if (luaL_newmetatable(L, "SOKOL_IMAGE")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "update", limage_update },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	p->img = sg_make_image(&img);
	p->width = img.width;
	p->height = img.height;
	return 1;
}

static int
lsampler(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	struct sampler * s = (struct sampler *)lua_newuserdatauv(L, sizeof(*s), 0);
	struct sg_sampler_desc desc = { 0 };
	if (lua_getfield(L, 1, "label") == LUA_TSTRING) {
		desc.label = lua_tostring(L, -1);
	}
	lua_pop(L, 1);
	// todo : set filter , etc
	s->handle = sg_make_sampler(&desc);
	return 1;
}

static int
ldraw(lua_State *L) {
	int base = luaL_checkinteger(L, 1);
	int n = luaL_checkinteger(L, 2);
	int inst = luaL_checkinteger(L, 3);
	sg_draw(base, n, inst);
	return 0;
}

static int
lsrbuffer_add(lua_State *L) {
	struct sr_buffer *b = (struct sr_buffer *)luaL_checkudata(L, 1, "SOLUNA_SRBUFFER");
	float scale = luaL_checknumber(L, 2);
	float rot = luaL_checknumber(L, 3);

	struct draw_primitive tmp;
	sprite_set_sr(&tmp, scale, rot);
	int index = srbuffer_add(b, tmp.sr);
	if (index < 0)
		return 0;
	lua_pushinteger(L, index);
	return 1;
}

static int
lsrbuffer_ptr(lua_State *L) {
	struct sr_buffer *b = (struct sr_buffer *)luaL_checkudata(L, 1, "SOLUNA_SRBUFFER");
	int sz;
	void * ptr = srbuffer_commit(b, &sz);
	if (ptr == NULL)
		return 0;
	lua_pushlightuserdata(L, ptr);
	lua_pushinteger(L, sz);
	return 2;
}

static int
lsrbuffer(lua_State *L) {
	struct sr_buffer *b = (struct sr_buffer *)lua_newuserdatauv(L, sizeof(*b), 0);
	srbuffer_init(b);
	if (luaL_newmetatable(L, "SOLUNA_SRBUFFER")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "add", lsrbuffer_add },
			{ "ptr", lsrbuffer_ptr },
			{ NULL, NULL },
		};
		luaL_setfuncs(L, l, 0);

		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

struct inst_object {
	float x, y;
	float sr_index;
};

struct sprite_object {
	uint32_t off;
	uint32_t u;
	uint32_t v;
};

#define MAX_OBJECT 32768

static void
submit_inst(lua_State *L, struct buffer *inst, struct sr_buffer *srb, struct draw_primitive *prim, int n) {
	struct inst_object tmp[MAX_OBJECT];
	int i;
	for (i=0;i<n;i++) {
		struct draw_primitive *p = &prim[i];
		int index = srbuffer_add(srb, p->sr);
		if (index < 0)
			luaL_error(L, "sr buffer is full");
		tmp[i].x = (float)p->x / 256.0f;
		tmp[i].y = (float)p->y / 256.0f;
		tmp[i].sr_index = (float)index;
	}
	sg_update_buffer(inst->handle, &(sg_range) { tmp, n * sizeof(tmp[0]) });
}

static int
submit_sprite(lua_State *L, struct buffer *sprite, struct sprite_bank * bank, struct draw_primitive *prim, int n, int *texid) {
	if (n == 0)
		return 0;
	struct sprite_rect * rect = bank->rect;
	int rect_n = bank->n;
	int index = prim[0].sprite - 1;
	if (index < 0 || index >= rect_n)
		return luaL_error(L, "Invalid sprite id %d", index);
	int tex = rect[index].texid;
	*texid = tex;

	struct sprite_object tmp[MAX_OBJECT];
	int i;
	for (i=0;i<n;i++) {
		struct draw_primitive *p = &prim[i];
		int index = p->sprite - 1;
		if (index < 0 || index >= rect_n)
			return luaL_error(L, "Invalid sprite id %d", index);
		struct sprite_rect *r = &rect[index];
		if (r->texid != tex)
			break;
		tmp[i].off = r->off;
		tmp[i].u = r->u;
		tmp[i].v = r->v;
	}

	sg_update_buffer(sprite->handle, &(sg_range) { tmp, i * sizeof(tmp[0]) });

	return i;
}

static int
lsubmit_batch(lua_State *L) {
	struct buffer *inst = (struct buffer *)luaL_checkudata(L, 1, "SOKOL_BUFFER");
	int inst_size = luaL_checkinteger(L, 2);
	struct buffer *sprite = (struct buffer *)luaL_checkudata(L, 3, "SOKOL_BUFFER");
	int sprite_size = luaL_checkinteger(L, 4);
	struct sr_buffer *srb = (struct sr_buffer *)luaL_checkudata(L, 5, "SOLUNA_SRBUFFER");
	struct sprite_bank * bank = (struct sprite_bank *)lua_touserdata(L, 6);
	struct draw_primitive *prim = (struct draw_primitive *)lua_touserdata(L, 7);
	int prim_n = luaL_checkinteger(L, 8);
	
	if (inst_size > MAX_OBJECT) {
		inst_size = MAX_OBJECT;
	}
	if (sprite_size < inst_size) {
		inst_size = sprite_size;
	}

	if (prim_n > inst_size) {
		prim_n = inst_size;
	}
	
	int texid;
	prim_n = submit_sprite(L, sprite, bank, prim, prim_n, &texid);
	submit_inst(L, inst, srb, prim, prim_n);

	lua_pushinteger(L, prim_n);
	lua_pushinteger(L, texid);

	return 2;
}

int
luaopen_render(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "pass", lpass_new },
		{ "submit", lsubmit },
		{ "pipeline", lpipeline },
		{ "image", limage },
		{ "buffer", lbuffer },
		{ "sampler", lsampler },
		{ "draw", ldraw },
		{ "srbuffer", lsrbuffer },
		{ "submit_batch", lsubmit_batch },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}