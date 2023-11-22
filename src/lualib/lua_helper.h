// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "lua.hpp"
#include <cstdint>
namespace tinynet {
namespace app {
class AppContainer;
}
}
LUA_API int	luaL_pushtraceback(lua_State *L, int idx);

LUA_API int luaL_poptraceback(lua_State *L, int idx);

LUA_API const char* luaL_strerror(lua_State *L, int err);

LUA_API int luaL_pcall(lua_State *L, int nargs, int nresults);

LUA_API void lua_setapp(lua_State *L, tinynet::app::AppContainer *app);

LUA_API tinynet::app::AppContainer* lua_getapp(lua_State* L);

#if(LUA_VERSION_NUM <= 501)
LUA_API lua_Integer luaL_len(lua_State *L, int index);
#endif
LUA_API int lua_isarray(lua_State *L, int idx);
