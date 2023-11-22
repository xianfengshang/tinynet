// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_timer.h"
#include "lua_helper.h"
#include "logging/logging.h"
#include "lua_compat.h"
#include "lua_script.h"
#include "lua_identifier.h"
#include "app/app_container.h"
#include <functional>

using namespace tinynet;

static void timer_on_timeout(lua_State* L, int nref) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, nref);
    if (!lua_isfunction(L, -1)) {
        log_warning("Timer can not found timeout callback function!");
        lua_pop(L, 1);
        return;
    }
    luaL_pcall(L, 0, 0);
}

static void timer_on_stop(lua_State *L, int nref) {
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
}

static int timer_stop(lua_State *L) {
    auto app = lua_getapp(L);
    int64_t guid = luaL_checkidentifier(L, 1);
    app->event_loop()->ClearTimer(guid);
    return 0;
}

static int timer_start(lua_State *L) {
    auto app = lua_getapp(L);
    uint64_t timeout = static_cast<uint64_t>(luaL_checkinteger(L, 1));
    uint64_t repeat = static_cast<uint64_t>(luaL_checkinteger(L, 2));
    luaL_argcheck(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function expected");
    lua_pushvalue(L, 3);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_State* mainL = app->get<lua_State>();
    auto ontimeout = std::bind(timer_on_timeout, mainL, nref);
    auto onstop = std::bind(timer_on_stop, mainL, nref);
    auto timer_guid = app->event_loop()->get_timer()->AddTimer(timeout, repeat, ontimeout, onstop);
    lua_pushidentifier(L, timer_guid);
    return 1;
}

static const luaL_Reg methods[] = {
    {"start", timer_start},
    {"stop", timer_stop },
    {0, 0}
};

LUALIB_API int luaopen_timer(lua_State *L) {
    luaL_newlib(L, methods);
    return 1;
}
