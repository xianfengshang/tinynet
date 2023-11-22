// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cstdint>
#include "lua.hpp"

LUA_API int luaL_isidentifier(lua_State *L, int index);

LUA_API void lua_pushidentifier(lua_State *L, int64_t guid);

LUA_API int64_t luaL_toidentifier(lua_State *L, int index);

LUA_API int64_t luaL_checkidentifier(lua_State *L, int index);
