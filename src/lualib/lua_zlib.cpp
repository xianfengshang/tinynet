// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_zlib.h"
#include "util/zlib_utils.h"
#include "base/io_buffer.h"

static int lua_deflate(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    unsigned char* in_data = reinterpret_cast<unsigned char*>(const_cast<char*>(data));
    tinynet::IOBuffer out_buf;
    if (ZlibUtils::deflate(in_data, len, &out_buf) == 0) {
        lua_pushlstring(L, out_buf.begin(), out_buf.size());
        return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, "deflate error");
    return 2;
}

static int lua_inflate(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    unsigned char* in_data = reinterpret_cast<unsigned char*>(const_cast<char*>(data));
    tinynet::IOBuffer out_buf;
    if (ZlibUtils::inflate(in_data, len, &out_buf) == 0) {
        lua_pushlstring(L, out_buf.begin(), out_buf.size());
        return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, "inflate error");
    return 2;
}

static const luaL_Reg methods[] = {
    {"deflate", lua_deflate},
    {"inflate", lua_inflate },
    {0, 0}
};

LUALIB_API int luaopen_zlib(lua_State *L) {
    luaL_newlib(L, methods);
    return 1;
}
