#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdint.h>

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_app.h"
#include "texquad.glsl.h"

#define UNIFORM_MAX 4

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

struct pipeline {
	sg_pipeline pip;
	struct uniform_desc uniform[UNIFORM_MAX];
};

static sg_pipeline
default_pipeline() {
	sg_shader shd = sg_make_shader(texquad_shader_desc(sg_query_backend()));

	sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc) {
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
	return pip;
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

static int
lpipeline(lua_State *L) {
	const char * name = luaL_checkstring(L, 1);
	struct pipeline * pip = (struct pipeline *)lua_newuserdatauv(L, sizeof(*pip), 0);
	memset(pip, 0, sizeof(*pip));
	if (strcmp(name, "default") == 0) {
		pip->pip = default_pipeline();
		pip->uniform[0] = (struct uniform_desc){ UB_vs_params , sizeof(vs_params_t) };
	} else {
		// todo : externl shaders and pipelines
		return luaL_error(L, "Invalid pipeline name");
	}
	if (luaL_newmetatable(L, "SOKOL_PIPELINE")) {
		luaL_Reg l[] = {
			{ "__index", NULL },
			{ "apply", lpipeline_apply },
			{ "uniform_slot", lpipe_uniform_slot },
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
luaopen_render(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "pass", lpass_new },
		{ "submit", lsubmit },
		{ "pipeline", lpipeline },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}