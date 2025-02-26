#include <lua.h>
#include <lauxlib.h>

int luaopen_ltask(lua_State *L);
int luaopen_ltask_root(lua_State *L);
int luaopen_ltask_bootstrap(lua_State* L);
int luaopen_embedsource(lua_State *L);
int luaopen_appmessage(lua_State *L);
int luaopen_applog(lua_State *L);
int luaopen_image(lua_State *L);
int luaopen_appinfo(lua_State *L);
int luaopen_render(lua_State *L);
int luaopen_spritemgr(lua_State *L);
int luaopen_datalist(lua_State *L);
int luaopen_soluna_file(lua_State *L);

void soluna_embed(lua_State* L) {
    static const luaL_Reg modules[] = {
        { "ltask", luaopen_ltask},
        { "ltask.root", luaopen_ltask_root},
        { "ltask.bootstrap", luaopen_ltask_bootstrap},
		{ "soluna.embedsource", luaopen_embedsource},
		{ "soluna.appmessage", luaopen_appmessage},
		{ "soluna.log", luaopen_applog },
		{ "soluna.image", luaopen_image },
		{ "soluna.appinfo", luaopen_appinfo },
		{ "soluna.render", luaopen_render },
		{ "soluna.spritemgr", luaopen_spritemgr },
		{ "soluna.datalist", luaopen_datalist },
		{ "soluna.file", luaopen_soluna_file },
//		{ "luaforward", luaopen_luaforward },
        { NULL, NULL },
    };

    const luaL_Reg *lib;
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    for (lib = modules; lib->func; lib++) {
        lua_pushcfunction(L, lib->func);
        lua_setfield(L, -2, lib->name);
    }
    lua_pop(L, 1);
}
