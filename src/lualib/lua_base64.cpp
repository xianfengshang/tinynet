// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_base64.h"
#include "base/crypto.h"
#include "lua_types.h"

static int lua_base64_encode(lua_State *L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    tinynet::string_view src(data, len);
    std::string dst =  Crypto::base64_encode(src);
    lua_pushlstring(L, dst.data(), dst.length());
    return 1;
}

static int lua_base64_decode(lua_State *L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    tinynet::string_view src(data, len);
    std::string dst = Crypto::base64_decode(src);
    lua_pushlstring(L, dst.data(), dst.length());
    return 1;
}

static const luaL_Reg methods[] = {
    { "encode", lua_base64_encode},
    { "decode", lua_base64_decode},
    { 0, 0}
};

LUALIB_API int luaopen_base64(lua_State *L) {
    luaL_newlib(L, methods);
    return 1;
}
