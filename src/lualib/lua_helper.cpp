// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <cstdio>
#include <cstring>
#include "logging/logging.h"
#include "lua_helper.h"
#include "lua_types.h"
#include "lua_compat.h"


#define  LUA_APP_KEY "_LUA_APP_KEY"

static int traceback(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    luaL_traceback(L, L, msg ? msg : "Error", 1);
    return 1;
}

int luaL_pushtraceback(lua_State *L, int idx) {
    lua_pushcfunction(L, traceback);
    lua_insert(L, idx);
    return idx;
}

int luaL_poptraceback(lua_State *L, int idx) {
    lua_remove(L, idx);
    return lua_gettop(L);
}

const char* luaL_strerror(lua_State *L, int err) {
    thread_local static char buffer[4 * 1024] = { 0 };
    switch (err) {
    case LUA_ERRRUN:
        snprintf(buffer, sizeof(buffer), "lua call error : %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : "");
        break;
    case LUA_ERRMEM:
        snprintf(buffer, sizeof(buffer), "lua memory error");
        break;
    case LUA_ERRERR:
        snprintf(buffer, sizeof(buffer), "lua error in error handler");
        break;
#if (LUA_VERSION_NUM > 501)
    case LUA_ERRSYNTAX:
        snprintf(buffer, sizeof(buffer), "lua syntax error ");
        break;
#endif
    default:
        buffer[0] = '\0';
    };
    return buffer;
}

int luaL_pcall(lua_State *L, int nargs, int nresults) {
    int base = lua_gettop(L) - nargs;
    int trace = luaL_pushtraceback(L, base);
    int err = lua_pcall(L, nargs, nresults, trace);
    luaL_poptraceback(L, trace);
    if (err != LUA_OK) {
        log_error("%s", luaL_strerror(L, err));
        lua_pop(L, 1);
    }
    return err;
}

void lua_setapp(lua_State *L, tinynet::app::AppContainer *ctx) {
    lua_pushstring(L, LUA_APP_KEY);
    lua_pushlightuserdata(L, ctx);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

tinynet::app::AppContainer* lua_getapp(lua_State* L) {
    lua_pushstring(L, LUA_APP_KEY);
    lua_rawget(L, LUA_REGISTRYINDEX);
    tinynet::app::AppContainer *ctx = (tinynet::app::AppContainer *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ctx;
}

#if(LUA_VERSION_NUM <= 501)
lua_Integer luaL_len(lua_State *L, int index) {
    return lua_objlen(L, index);
}
#endif
LUA_API int lua_isarray(lua_State *L, int idx) {
    int tabIdx = idx;
    if (!lua_istable(L, tabIdx)) return 0;
#if(LUA_VERSION_NUM <= 501)
    size_t len = lua_objlen(L, tabIdx);
#else
    size_t len = lua_rawlen(L, tabIdx);
#endif
    if (len == 0) return 0;
    size_t nkeys = 0;
    lua_pushnil(L);
    if (tabIdx < 0) tabIdx--;
    while (lua_next(L, tabIdx)) {
        nkeys++;
        lua_pop(L, 1);
    }
    return len == nkeys;
}