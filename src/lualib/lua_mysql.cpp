// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_mysql.h"
#include "mysql/mysql_client.h"
#include <memory>
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"
#include "lua_proto_types.h"

#define  MYSQL_META_TABLE "mysql_meta_table"

static mysql::MysqlClient* luaL_checkmysql(lua_State *L, int idx) {
    return (mysql::MysqlClient*)luaL_checkudata(L, idx, MYSQL_META_TABLE);
}

static void mysql_execute_callback(lua_State* L, int nref, const mysql::QueryResponse &response) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, nref);
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    if (!lua_isfunction(L, -1)) {
        log_warning("Mysql execute result can not found callback function!");
        lua_pop(L, 1);
        return;
    }
    int nargs;
    if (response.code) {
        nargs = 2;
        lua_pushnil(L);
        lua_pushstring(L, response.msg.c_str());
    } else {
        nargs = 1;
        LuaState S{ L };
        if (response.results.size() == 1) {
            S << response.results[0]; //Single result set
        } else {
            S << response.results;	//Multi result set
        }
    }
    luaL_pcall(L, nargs, 0);
}

static int mysql_execute(lua_State *L) {
    auto app = lua_getapp(L);
    auto LL = app->get<lua_State>();
    auto mysql_client = luaL_checkmysql(L, 1);
    size_t len = 0;
    const char* buf = luaL_checklstring(L, 2, &len);
    luaL_argcheck(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function expected");
    lua_pushvalue(L, 3);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    auto callback = std::bind(mysql_execute_callback, LL, nref, std::placeholders::_1);
    mysql_client->Query(buf, len, callback);
    return 0;
}

static int mysql_new(lua_State *L) {
    auto app = lua_getapp(L);
    luaL_argcheck(L, lua_type(L, 1) == LUA_TTABLE, 1, "table expected");
    mysql::Config config;
    LuaState S = { L };
    S >> config;
    auto ptr = lua_newuserdata(L, sizeof(mysql::MysqlClient));
    auto mysql_client = new(ptr) mysql::MysqlClient(app->event_loop());
    mysql_client->Init(config);
    luaL_getmetatable(L, MYSQL_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int mysql_delete(lua_State *L) {
    mysql::MysqlClient *mysql_client = (mysql::MysqlClient *)lua_touserdata(L, 1);
    mysql_client->Shutdown();
    mysql_client->~MysqlClient();
    return 0;
}

static const luaL_Reg meta_methods[] = {
    { "execute", mysql_execute },
    { "__gc", mysql_delete },
    { 0, 0 }
};
static const luaL_Reg methods[] = {
    { "new", mysql_new },
    { 0, 0 }
};

LUALIB_API int luaopen_mysql(lua_State *L) {
    luaL_newmetatable(L, MYSQL_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
