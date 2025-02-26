#define SOKOL_IMPL
#define SOKOL_D3D11

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <stdio.h>

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_args.h"
#include "message.h"
#include "loginfo.h"

struct app_context {
	lua_State *L;
	void (*send_message)(void *ud, void *p);
	void *send_message_ud;
	int (*send_log)(void *ud, unsigned int id, void *data, uint32_t sz);
	void *send_log_ud;
};

static struct app_context *CTX = NULL;

static void
send_app_message(void *p) {
	if (CTX && CTX->send_message) {
		CTX->send_message(CTX->send_message_ud, p);
	}
}

static int
set_callback(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 4, LUA_TLIGHTUSERDATA);
	CTX->send_message = lua_touserdata(L, 1);
	CTX->send_message_ud = lua_touserdata(L, 2);
	CTX->send_log = lua_touserdata(L, 3);
	CTX->send_log_ud = lua_touserdata(L, 4);
	return 0;
}

void soluna_openlibs(lua_State *L);

static const char *code = "local embed = require 'soluna.embedsource' ; local f = load(embed.runtime.main()) ; f(...)";

static int
pmain(lua_State *L) {
	soluna_openlibs(L);
	lua_pushcfunction(L, set_callback);
	lua_setglobal(L, "external_messsage");
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
	int arg_n = lua_gettop(L) - arg_table + 1;
	if (luaL_loadstring(L, code) != LUA_OK) {
		return lua_error(L);
	}
	lua_insert(L, -arg_n-1);
	if (lua_pcall(L, arg_n, 0, 0) != LUA_OK) {
		return lua_error(L);
	}
	
	return 0;
}

static void
start_app(lua_State *L) {
	lua_pushcfunction(L, pmain);
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error : %s", lua_tostring(L, -1));
		lua_close(L);
		sapp_quit();
	}
}

static void
log_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr, const char* filename, void* user_data) {
	if (CTX == NULL || CTX->send_log == NULL)
		return;
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

static void
app_init() {
	static struct app_context app;
	lua_State *L = luaL_newstate();
	if (L == NULL)
		return;

	app.L = L;
	app.send_message = NULL;
	app.send_message_ud = NULL;
	app.send_log = NULL;
	app.send_log_ud = NULL;
	
	CTX = &app;
	
	sg_setup(&(sg_desc) {
        .environment = sglue_environment(),
        .logger.func = log_func,			
	});
	start_app(L);
	sargs_shutdown();
}

static void
app_frame() {
	send_app_message(message_create64("frame", sapp_frame_count()));
}

static int
pcleanup(lua_State *L) {
	if (lua_getglobal(L, "cleanup") != LUA_TFUNCTION)
		return 0;
	lua_call(L, 0, 0);
	return 0;
}

static void
app_cleanup() {
	if (CTX == NULL)
		return;
	lua_State *L = CTX->L;
	send_app_message(message_create("cleanup", 0, 0));
	lua_pushcfunction(L, pcleanup);
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error: %s", lua_tostring(L, -1));
	}
	lua_close(L);
	memset(CTX, 0, sizeof(*CTX));
	sg_shutdown();
}

static void
mouse_message(const sapp_event* ev) {
	const char *typestr = NULL;
	int p1 = 0;
	int p2 = 0;
	switch (ev->type) {
	case SAPP_EVENTTYPE_MOUSE_MOVE:
		typestr = "mouse_move";
		p1 = ev->mouse_x;
		p2 = ev->mouse_y;
		break;
	case SAPP_EVENTTYPE_MOUSE_DOWN:
	case SAPP_EVENTTYPE_MOUSE_UP:
		typestr = "mouse_button";
		p1 = ev->mouse_button;
		p2 = ev->type == SAPP_EVENTTYPE_MOUSE_DOWN;
		break;
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
		typestr = "mouse_scroll";
		p1 = ev->scroll_y;
		p2 = ev->scroll_x;
		break;
	default:
		typestr = "mouse";
		p1 = ev->type;
		break;
	}
	send_app_message(message_create(typestr, p1, p2));
}

static void
window_message(const sapp_event *ev) {
	const char *typestr = NULL;
	int p1 = 0;
	int p2 = 0;
	switch (ev->type) {
	case SAPP_EVENTTYPE_RESIZED:
		typestr = "window_resize";
		p1 = ev->window_width;
		p2 = ev->window_height;
		break;
	default:
		typestr = "window";
		p1 = ev->type;
		break;
	}
	send_app_message(message_create(typestr, p1, p2));
}

static void
app_event(const sapp_event* ev) {
	switch (ev->type) {
	case SAPP_EVENTTYPE_MOUSE_MOVE:
	case SAPP_EVENTTYPE_MOUSE_DOWN:
	case SAPP_EVENTTYPE_MOUSE_UP:
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
	case SAPP_EVENTTYPE_MOUSE_ENTER:
	case SAPP_EVENTTYPE_MOUSE_LEAVE:
		mouse_message(ev);
		break;
	case SAPP_EVENTTYPE_RESIZED:
		window_message(ev);
		break;
	default:
		send_app_message(message_create("message", ev->type, 0));
		break;
	}
}

static int
lappinfo_width(lua_State *L) {
	lua_pushinteger(L, sapp_width());
	return 1;
}

static int
lappinfo_height(lua_State *L) {
	lua_pushinteger(L, sapp_height());
	return 1;
}

int
luaopen_appinfo(lua_State *L) {
	lua_newtable(L);
	luaL_Reg l[] = {
		{ "width", lappinfo_width },
		{ "height", lappinfo_height },
		{ NULL , NULL },
	};
	luaL_newlib(L, l);
	return 1;
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
