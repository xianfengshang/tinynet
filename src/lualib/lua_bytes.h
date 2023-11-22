// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "lua.hpp"
#include <string>

LUA_API int luaL_isbytes(lua_State *L, int index);

LUA_API void lua_pushlbytes(lua_State *L, const char* data, size_t len);

LUA_API void lua_pushbytes(lua_State *L, const std::string* s);

LUA_API void lua_pushandswapbytes(lua_State *L, std::string* s);

LUA_API std::string* luaL_tobytes(lua_State *L, int index);

LUA_API std::string* luaL_tobytes(lua_State *L, int index, std::string* scratch);

LUA_API std::string* luaL_checkbytes(lua_State *L, int index);

LUA_API int luaopen_bytes(lua_State *L);


