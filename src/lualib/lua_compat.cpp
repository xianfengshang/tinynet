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
#include "lua_compat.h"
#include <cmath>
#ifdef LUA_VERSION_NUM
#if (LUA_VERSION_NUM <= 501)
LUA_API void  lua_rawsetp(lua_State *L, int idx, const void *p) {
    lua_checkstack(L, 1);
    lua_pushlightuserdata(L, (void*)p);
    lua_insert(L, -2);
    lua_rawset(L, idx);
}
LUA_API int lua_rawgetp(lua_State *L, int idx, const void *p) {
    lua_pushlightuserdata(L, (void*)p);
    lua_rawget(L, idx);
    return lua_type(L, -1);
}
LUA_API int lua_absindex(lua_State *L, int i) {
    if (i < 0 && i > LUA_REGISTRYINDEX)
        i += lua_gettop(L) + 1;
    return i;
}
LUALIB_API int luaL_getsubtable(lua_State *L, int idx, const char *fname) {
    lua_getfield(L, idx, fname);
    if (lua_type(L, -1) == LUA_TTABLE)
        return 1;  /* table already there */
    else {
        lua_pop(L, 1);  /* remove previous result */
        idx = lua_absindex(L, idx);
        lua_newtable(L);
        lua_pushvalue(L, -1);  /* copy to be left at top */
        lua_setfield(L, idx, fname);  /* assign new table to field */
        return 0;  /* false, because did not find table there */
    }
}
LUALIB_API void luaL_requiref(lua_State *L, const char *modname,
                              lua_CFunction openf, int glb) {
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_getfield(L, -1, modname);  /* LOADED[modname] */
    if (!lua_toboolean(L, -1)) {  /* package not already loaded? */
        lua_pop(L, 1);  /* remove field */
        lua_pushcfunction(L, openf);
        lua_pushstring(L, modname);  /* argument to open function */
        lua_call(L, 1, 1);  /* call 'openf' to open module */
        lua_pushvalue(L, -1);  /* make copy of module (call result) */
        lua_setfield(L, -3, modname);  /* LOADED[modname] = module */
    }
    lua_remove(L, -2);  /* remove LOADED table */
    if (glb) {
        lua_pushvalue(L, -1);  /* copy of module */
        lua_setglobal(L, modname);  /* _G[modname] = module */
    }
}

LUA_API int lua_isinteger(lua_State *L, int idx) {
    int type = lua_type(L, idx);
    if (type != LUA_TNUMBER) return 0;
    auto value = lua_tonumber(L, idx);
    return std::trunc(value) == value;
}

#if !LUAJIT_VERSION_NUM
void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
    luaL_checkstack(L, nup + 1, "too many upvalues");
    for (; l->name != NULL; l++) {  /* fill the table with given functions */
        int i;
        lua_pushstring(L, l->name);
        for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(L, -(nup + 1));
        lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
        lua_settable(L, -(nup + 3)); /* table must be below the upvalues, the name and the closure */
    }
    lua_pop(L, nup);  /* remove upvalues */
}

LUALIB_API void luaL_traceback(lua_State *L, lua_State *L1, const char *msg, int level) {
    (void)L1;
    lua_getglobal(L, "debug");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return;
    }
    lua_remove(L, -2);

    if (msg) {
        lua_pushstring(L, msg);
    }

    lua_pushinteger(L, 1 + level);	/* skip debug.traceback function */
    lua_call(L, 2, 1);				/* call debug.traceback */
}
#endif
#endif
#endif