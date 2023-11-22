/******************************************************************************
* Copyright (C) 1994-2012 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
#ifndef __LUACOMPAT_H
#define  __LUACOMPAT_H
#include "lua.hpp"
#ifdef LUA_VERSION_NUM
#if (LUA_VERSION_NUM <= 501)

#if !LUAJIT_VERSION_NUM
#define LUA_OK 0
#endif

#define LUA_RIDX_MAINTHREAD	1
#define LUA_RIDX_GLOBALS	2
#define LUA_RIDX_LAST		LUA_RIDX_GLOBALS

#define LUA_LOADED_TABLE	"_LOADED"

#ifdef __cplusplus
extern "C" {
#endif
LUA_API void  (lua_rawsetp)(lua_State *L, int idx, const void *p);

LUA_API int (lua_rawgetp)(lua_State *L, int idx, const void *p);

LUA_API void luaL_requiref(lua_State *L, const char *modname,
                           lua_CFunction openf, int glb);

LUA_API int lua_absindex(lua_State *L, int i);

LUALIB_API int luaL_getsubtable(lua_State *L, int idx, const char *fname);

LUALIB_API void luaL_requiref(lua_State *L, const char *modname,
                              lua_CFunction openf, int glb);

LUA_API int             (lua_isinteger)(lua_State *L, int idx);

#if !LUAJIT_VERSION_NUM
void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup);

LUALIB_API void luaL_traceback(lua_State *L, lua_State *L1, const char *msg, int level);
#endif
#ifdef __cplusplus
}
#endif

#define lua_pushglobaltable(L)  \
	lua_pushvalue(L, LUA_GLOBALSINDEX)

#define luaL_newlibtable(L, l) \
	lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)
#define luaL_newlib(L, l)	(luaL_newlibtable(L, l), luaL_setfuncs(L, l, 0))

#endif
#endif
#endif