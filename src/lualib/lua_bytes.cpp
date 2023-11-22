// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_bytes.h"
#include "lua_types.h"
#include <string>
#include "util/string_utils.h"

#define STD_STRING_META_TABLE "std_string_meta_table"

int luaL_isbytes(lua_State *L, int index) {
    return luaL_testudata(L, index, STD_STRING_META_TABLE) ? 1 : 0;
}

void lua_pushlbytes(lua_State *L, const char *data, size_t len) {
    std::string *ud = (std::string *)lua_newuserdata(L, sizeof(std::string));
    new (ud) std::string(data, len);
    luaL_getmetatable(L, STD_STRING_META_TABLE);
    lua_setmetatable(L, -2);
}

void lua_pushbytes(lua_State *L, const std::string *s) {
    std::string *ud = (std::string *)lua_newuserdata(L, sizeof(std::string));
    new (ud) std::string(*s);
    luaL_getmetatable(L, STD_STRING_META_TABLE);
    lua_setmetatable(L, -2);
}

void lua_pushandswapbytes(lua_State *L, std::string *s) {
    std::string *ud = (std::string *)lua_newuserdata(L, sizeof(std::string));
    new (ud) std::string();
    s->swap(*ud);
    luaL_getmetatable(L, STD_STRING_META_TABLE);
    lua_setmetatable(L, -2);
}

std::string *luaL_tobytes(lua_State *L, int index) {
    return (std::string *)luaL_testudata(L, index, STD_STRING_META_TABLE);
}

std::string *luaL_checkbytes(lua_State *L, int index) {
    return (std::string *)luaL_checkudata(L, index, STD_STRING_META_TABLE);
}

std::string *luaL_tobytes(lua_State *L, int index, std::string *scratch) {
    if (!scratch)
        return NULL;

    int type = lua_type(L, index);
    if (type == LUA_TSTRING) {
        size_t len = 0;
        const char *bin = lua_tolstring(L, index, &len);
        scratch->assign(bin, len);
    } else if (type == LUA_TUSERDATA) {
        std::string *s = luaL_tobytes(L, index);
        if (s) {
            return s;
        }
    }
    return scratch;
}

static int lua_bytes_get_data(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    lua_pushlstring(L, s->data(), s->length());
    return 1;
}

static int lua_bytes_get_size(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    lua_pushinteger(L, static_cast<lua_Integer>(s->size()));
    return 1;
}

static int lua_bytes_append(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    s->append(data, len);
    return 0;
}

static int lua_bytes_resize(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    int size = luaL_optint(L, 2, 0);
    s->resize(size);
    return 0;
}

static int lua_bytes_reserve(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    int size = luaL_optint(L, 2, 0);
    s->reserve(size);
    return 0;
}

static int lua_bytes_clear(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    s->clear();
    return 0;
}

static int lua_bytes_at(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    int index = (int)luaL_checkinteger(L, 2) - 1;
    if (index < 0 || index > (int)s->size()) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, (*s)[index]);
    }
    return 1;
}

static int lua_bytes_new(lua_State *L) {
    size_t len;
    const char *data = luaL_optlstring(L, 1, NULL, &len);
    std::string *ud = (std::string *)lua_newuserdata(L, sizeof(std::string));
    new (ud) std::string();
    if (data != NULL) {
        ud->assign(data, len);
    }
    luaL_getmetatable(L, STD_STRING_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_bytes_delete(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    s->~basic_string();
    return 0;
}

static int lua_bytes_tostring(lua_State *L) {
    std::string *s = luaL_checkbytes(L, 1);
    auto info = StringUtils::dump_hex(*s);
    lua_pushlstring(L, info.data(), info.size());
    return 1;
}

static int lua_bytes_dump(lua_State *L) {
    std::string scratch;
    auto data = luaL_tobytes(L, 1, &scratch);
    if (data != NULL) {
        auto info = StringUtils::dump_hex(*data);
        lua_pushlstring(L, info.data(), info.size());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static const luaL_Reg bytes_meta_methos[] = {
    {"data", lua_bytes_get_data},
    {"size", lua_bytes_get_size},
    {"append", lua_bytes_append},
    {"resize", lua_bytes_resize},
    {"reserve", lua_bytes_reserve},
    {"clear", lua_bytes_clear},
    {"byte", lua_bytes_at},
    {"dump", lua_bytes_tostring},
    {"__len", lua_bytes_get_size },
    {"__tostring", lua_bytes_tostring},
    {"__gc", lua_bytes_delete},
    {0, 0}
};

static const luaL_Reg bytes_methods[] = {
    {"new", lua_bytes_new},
    {"dump", lua_bytes_dump},
    {0, 0}
};

int luaopen_bytes(lua_State *L) {
    luaL_newmetatable(L, STD_STRING_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, bytes_meta_methos, 0);
    lua_pop(L, 1);

    luaL_newlib(L, bytes_methods);
    return 1;
}
