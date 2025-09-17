#define SOKOL_IMPL

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#define SOKOL_D3D11

#elif defined(__APPLE__)

#define SOKOL_METAL

#elif defined(__linux__)

#define SOKOL_GLES3

#else

#error Unsupport platform

#endif

#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdio.h>

#define FRAME_CALLBACK 1
#define CLEANUP_CALLBACK 2
#define EVENT_CALLBACK 3
#define CALLBACK_COUNT 3

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_args.h"
#include "loginfo.h"
#include "appevent.h"

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#define PLATFORM "windows"

#elif defined(__APPLE__)

#define PLATFORM "macos"

#elif defined(__linux__)

#define PLATFORM "linux"

#else

#define PLATFORM "unknown"

#endif


struct app_context {
	lua_State *L;
	lua_State *quitL;
	int (*send_log)(void *ud, unsigned int id, void *data, uint32_t sz);
	void *send_log_ud;
	void *mqueue;
};

static struct app_context *CTX = NULL;

struct soluna_message {
	const char * type;
	union {
		int p[2];
		uint64_t u64;
	} v;
};

static inline struct soluna_message *
message_create(const char *type, int p1, int p2) {
	struct soluna_message * msg = (struct soluna_message *)malloc(sizeof(*msg));
	msg->type = type;
	msg->v.p[0] = p1;
	msg->v.p[1] = p2;
	return msg;
}

static inline struct soluna_message *
message_create64(const char *type, uint64_t p) {
	struct soluna_message * msg = (struct soluna_message *)malloc(sizeof(*msg));
	msg->type = type;
	msg->v.u64 = p;
	return msg;
}

static inline void
message_release(struct soluna_message *msg) {
	free(msg);
}

static int
lmessage_send(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	void (*send_message)(void *ud, void *p) = lua_touserdata(L, 1);
	void *send_message_ud = lua_touserdata(L, 2);
	const char * what = NULL;
	if (lua_type(L, 3) == LUA_TSTRING) {
		what = lua_tostring(L, 3);
	} else {
		luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);
		what = (const char *)lua_touserdata(L, 3);
	}
	int64_t p1 = luaL_optinteger(L, 4, 0);
	struct soluna_message * msg = NULL;
	if (lua_isnoneornil(L, 5)) {
		msg = message_create64(what, p1);
	} else {
		int p2 = luaL_checkinteger(L, 5);
		msg = message_create(what, (int)p1, p2);
	}
	send_message(send_message_ud, msg);
	return 0;
}

static int
lmessage_unpack(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	struct soluna_message *m = (struct soluna_message *)lua_touserdata(L,1);
	lua_pushstring(L, m->type);
	lua_pushinteger(L, m->v.p[0]);
	lua_pushinteger(L, m->v.p[1]);
	lua_pushinteger(L, m->v.u64);
	message_release(m);
	return 4;
}

static int
lquit_signal(lua_State *L) {
	if (CTX) {
		CTX->quitL = CTX->L;
		CTX->L = NULL;
	}
	return 0;
}

static int
levent_unpack(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	struct event_message em;
	app_event_unpack(&em, lua_touserdata(L, 1));
	lua_pushlightuserdata(L, (void *)em.typestr);
	lua_pushinteger(L, em.p1);
	lua_pushinteger(L, em.p2);
	return 3;
}

static int
lset_window_title(lua_State *L) {
	if (CTX == NULL || lua_type(L, 1) != LUA_TSTRING)
		return 0;
	const char * text = lua_tostring(L, 1);
	sapp_set_window_title(text);
	return 0;
}

static int
lclose_window(lua_State *L) {
	sapp_quit();
	return 0;
}

static int
lmqueue(lua_State *L) {
	if (CTX == NULL || CTX->mqueue == NULL) {
		return luaL_error(L, "Not init mqueue");
	}
	lua_pushlightuserdata(L, CTX->mqueue);
	return 1;
}

static int
lcontext_acquire(lua_State *L) {
#if defined(__linux__)
  eglMakeCurrent(_sapp.egl.display, _sapp.egl.surface, _sapp.egl.surface, _sapp.egl.context);
#endif
  return 0;
}

static int
lcontext_release(lua_State *L) {
#if defined(__linux__)
  eglMakeCurrent(_sapp.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
#endif
  return 0;
}

int
luaopen_soluna_app(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "context_acquire", lcontext_acquire },
		{ "context_release", lcontext_release },
		{ "mqueue", lmqueue },
		{ "unpackmessage", lmessage_unpack },
		{ "sendmessage", lmessage_send },
		{ "unpackevent", levent_unpack },
		{ "set_window_title", lset_window_title },
		{ "quit", lquit_signal },
		{ "close_window", lclose_window },
		{ "platform", NULL },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	lua_pushliteral(L, PLATFORM);
	lua_setfield(L, -2, "platform");

	return 1;
}

static void
log_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr, const char* filename, void* user_data) {
	if (CTX == NULL || CTX->send_log == NULL) {
		fprintf(stderr, "%s (%d) : %s\n", filename, line_nr, message);
		return;
	}
	struct log_info *msg = (struct log_info *)malloc(sizeof(*msg));
	if (tag) {
		strncpy(msg->tag, tag, sizeof(msg->tag));
		msg->tag[sizeof(msg->tag)-1] = 0;
	} else {
		msg->tag[0] = 0;
	}
	msg->log_level = log_level;
	msg->log_item = log_item;
	msg->line_nr = line_nr;
	if (message) {
		strncpy(msg->message, message, sizeof(msg->message));
		msg->message[sizeof(msg->message)-1] = 0;
	} else {
		msg->message[0] = 0;
	}
	msg->filename = filename;
	CTX->send_log(CTX->send_log_ud, 0, msg, sizeof(*msg));
}

void soluna_openlibs(lua_State *L);

static const char *code = "local embed = require 'soluna.embedsource' ; local f = load(embed.runtime.main()) ; return f(...)";

static void
set_app_info(lua_State *L, int index) {
	lua_newtable(L);
	lua_pushinteger(L, sapp_width());
	lua_setfield(L, -2, "width");
	lua_pushinteger(L, sapp_height());
	lua_setfield(L, -2, "height");
	lua_setfield(L, index, "app");
}

static int
pmain(lua_State *L) {
	soluna_openlibs(L);
	int n = sargs_num_args();
	luaL_checkstack(L, n+1, NULL);
	int i;
	lua_newtable(L);
	int arg_table = lua_gettop(L);
	for (i=0;i<n;i++) {
		const char *k = sargs_key_at(i);
		const char *v = sargs_value_at(i);
		if (v[0] == 0) {
			lua_pushstring(L, sargs_key_at(i));
		} else {
			lua_pushstring(L, v);
			lua_setfield(L, arg_table, k);
		}
	}
	set_app_info(L, arg_table);
	
	int arg_n = lua_gettop(L) - arg_table + 1;
	if (luaL_loadstring(L, code) != LUA_OK) {
		return lua_error(L);
	}
	lua_insert(L, -arg_n-1);
	lua_call(L, arg_n, 1);
	return 1;
}

static void *
get_ud(lua_State *L, const char *key) {
	if (lua_getfield(L, -1, key) != LUA_TLIGHTUSERDATA) {
		lua_pop(L, 1);
		return NULL;
	}
	void * ud = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return ud;
}

static int
get_function(lua_State *L, const char *key, int index) {
	if (lua_getfield(L, -1, key) != LUA_TFUNCTION) {
		fprintf(stderr, "main.lua need return a %s function", key);
		return 1;
	}
	lua_insert(L, index);
	return 0;
}

static int
init_callback(lua_State *L, struct app_context * ctx) {
	if (!lua_istable(L, -1)) {
		fprintf(stderr, "main.lua need return a table, it's %s\n", lua_typename(L, lua_type(L, -1)));
		return 1;
	}
	ctx->send_log = get_ud(L, "send_log");
	ctx->send_log_ud = get_ud(L, "send_log_ud");
	ctx->mqueue = get_ud(L, "mqueue");
	if (get_function(L, "frame", FRAME_CALLBACK))
		return 1;
	if (get_function(L, "cleanup", CLEANUP_CALLBACK))
		return 1;
	if (get_function(L, "event", EVENT_CALLBACK))
		return 1;
	lua_settop(L, CALLBACK_COUNT);
	return 0;
}

static int
msghandler(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	luaL_traceback(L, L, msg, 1);
	return 1;
}

static int
start_app(lua_State *L) {
	lua_settop(L, 0);
	lua_pushcfunction(L, msghandler);
	lua_pushcfunction(L, pmain);
	if (lua_pcall(L, 0, 1, 1) != LUA_OK) {
		fprintf(stderr, "Start fatal : %s", lua_tostring(L, -1));
		return 1;
	} else {
		return init_callback(L, CTX);
	}
}

static void
app_init() {
	static struct app_context app;
	lua_State *L = luaL_newstate();
	if (L == NULL)
		return;

	app.L = L;
	app.quitL = NULL;
	app.send_log = NULL;
	app.send_log_ud = NULL;
	app.mqueue = NULL;
	
	CTX = &app;
	
	sg_setup(&(sg_desc) {
        .environment = sglue_environment(),
        .logger.func = log_func,			
	});
	if (start_app(L)) {
		sargs_shutdown();
		lua_close(L);
		app.L = NULL;
		sapp_quit();
	} else {
		sargs_shutdown();
	}
}

static lua_State *
get_L(struct app_context *ctx) {
	if (ctx == NULL)
		return NULL;
	lua_State *L = ctx->L;
	if (L == NULL) {
		if (ctx->quitL != NULL) {
			ctx->L = ctx->quitL;
			ctx->quitL = NULL;
			sapp_quit();
			return NULL;
		}
	}
	return L;
}

static void
invoke_callback(lua_State *L, int index, int nargs) {
	lua_pushvalue(L, index);
	if (nargs > 0) {
		lua_insert(L, -nargs-1);
	}
	if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error : %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}

static void
app_frame() {
	lua_State *L = get_L(CTX);
	if (L) {
		lua_pushinteger(L, sapp_frame_count());
		invoke_callback(L, FRAME_CALLBACK, 1);
	}
}

static void
app_cleanup() {
	lua_State *L = get_L(CTX);
	if (L) {
		invoke_callback(L, CLEANUP_CALLBACK, 0);
	}
	sg_shutdown();
}

static void
app_event(const sapp_event* ev) {
	lua_State *L = get_L(CTX);
	if (L) {
		lua_pushlightuserdata(L, (void *)ev);
		invoke_callback(L, EVENT_CALLBACK, 1);
	}
}

sapp_desc
sokol_main(int argc, char* argv[]) {
	sargs_desc arg_desc;
	memset(&arg_desc, 0, sizeof(arg_desc));
	arg_desc.argc = argc;
	arg_desc.argv = argv;
	sargs_setup(&arg_desc);
	
	sapp_desc d;
	memset(&d, 0, sizeof(d));
		
	d.width = 1024;
	d.height = 768;
	d.init_cb = app_init;
	d.frame_cb = app_frame;
	d.cleanup_cb = app_cleanup;
	d.event_cb = app_event;
	d.logger.func = log_func;
	d.win32_console_utf8 = 1;
	d.win32_console_attach = 1;
	d.window_title = "soluna";
	d.alpha = 0;

	return d;
}
