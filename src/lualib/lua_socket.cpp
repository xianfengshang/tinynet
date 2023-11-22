// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_socket.h"
#include "lua_tcp.h"

static const luaL_Reg socket_members[] {
    {"tcp", luaopen_tcp},
    {0, 0}
};

LUALIB_API int luaopen_socket(lua_State *L) {
    const luaL_Reg* lib;
    //register socket libs
    lua_newtable(L);
    int ret;
    for (lib = socket_members; lib->func; lib++) {
        ret = lib->func(L);
        if (ret == 1) {
            lua_setfield(L, -2, lib->name);
        } else {
            lua_pop(L, ret);
        }
    }
    return 1;
}
