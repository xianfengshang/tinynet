// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_identifier.h"
#include <string>
LUA_API int luaL_isidentifier(lua_State *L, int index) {
    int type = lua_type(L, index);
    int result = type == LUA_TNUMBER || type == LUA_TSTRING;
    return result;
}

#if (LUA_VERSION_NUM <= 501)
union lua_Identifier {
    double d_data;
    int64_t i_data;
};

void lua_pushidentifier(lua_State *L, int64_t guid) {
    lua_Identifier id;
    id.i_data = guid;
    lua_pushnumber(L, id.d_data);
}

int64_t luaL_toidentifier(lua_State *L, int index) {
    lua_Identifier id;
    int type = lua_type(L, index);
    if (type == LUA_TNUMBER) {
        id.d_data = lua_tonumber(L, index);
    } else if (type == LUA_TSTRING) {
        const char * str = lua_tostring(L, 1);
        id.i_data = strtoll(str, NULL, 10);
    } else {
        id.i_data = 0;
    }
    return id.i_data;
}

int64_t luaL_checkidentifier(lua_State *L, int index) {
    lua_Identifier id;
    int type = lua_type(L, index);
    if (type == LUA_TNUMBER) {
        id.d_data = lua_tonumber(L, index);
    } else if (type == LUA_TSTRING) {
        const char * str = lua_tostring(L, index);
        id.i_data = strtoll(str, NULL, 10);
    } else {
        id.i_data = 0;	//fix gcc warning
        luaL_argerror(L, index, "unique id expected");
    }
    return id.i_data;
}
#else

void lua_pushidentifier(lua_State *L, int64_t guid) {
    lua_pushinteger(L, guid);
}

int64_t luaL_toidentifier(lua_State *L, int index) {
    int64_t id;
    int type = lua_type(L, index);
    if (type == LUA_TNUMBER) {
        id = lua_tointeger(L, index);
    } else if (type == LUA_TSTRING) {
        const char * str = lua_tostring(L, 1);
        id = strtoll(str, NULL, 10);
    } else {
        id = 0;
    }
    return id;
}

int64_t luaL_checkidentifier(lua_State *L, int index) {
    int64_t id;
    int type = lua_type(L, index);
    if (type == LUA_TNUMBER) {
        id = lua_tointeger(L, index);
    } else if (type == LUA_TSTRING) {
        const char * str = lua_tostring(L, index);
        id = strtoll(str, NULL, 10);
    } else {
        id = 0;	//fix gcc warning
        luaL_argerror(L, index, "unique id expected");
    }
    return id;
}
#endif