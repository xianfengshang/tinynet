// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifndef __LUATINY_H
#define __LUATINY_H
#include "lua.hpp"

LUALIB_API int luaopen_tinynet(lua_State *L);
LUALIB_API void luaL_opentinynet(lua_State *L);
#endif
