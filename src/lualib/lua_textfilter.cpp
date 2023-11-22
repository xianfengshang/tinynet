// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <memory>
#include <functional>
#include <algorithm>
#include <map>
#include "lua_textfilter.h"
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"
#include "lua_proto_types.h"
#include "text/text_filter.h"

#define  TEXT_FILTER_META "text_filter_meta_table"

static tinynet::text::TextFilter* luaL_checktextfilter(lua_State *L, int idx) {
    return (tinynet::text::TextFilter*)luaL_checkudata(L, idx, TEXT_FILTER_META);
}

static int text_filter_new(lua_State *L) {
    auto filter = lua_newuserdata(L, sizeof(tinynet::text::TextFilter));
    int type = luaL_optint(L, 1, 0);
    new(filter) tinynet::text::TextFilter(type);
    luaL_getmetatable(L, TEXT_FILTER_META);
    lua_setmetatable(L, -2);
    return 1;
}

static int text_filter_delete(lua_State *L) {
    tinynet::text::TextFilter* filter = luaL_checktextfilter(L, 1);
    filter->~TextFilter();
    return 0;
}

static int text_filter_init(lua_State *L) {
    tinynet::text::TextFilter* filter = luaL_checktextfilter(L, 1);
    std::vector<std::string> dict;
    LuaState S{ L };
    S >> dict;
    filter->Init(dict);
    return 0;
}


static int text_filter_contains_key(lua_State *L) {
    tinynet::text::TextFilter* filter = luaL_checktextfilter(L, 1);
    size_t len;
    const char* words = luaL_checklstring(L, 2, &len);
    std::string in_words(words, len);
    lua_pushboolean(L, filter->ContainsKey(in_words));
    return 1;
}

static int text_filter_contains(lua_State *L) {
    tinynet::text::TextFilter* filter = luaL_checktextfilter(L, 1);
    size_t len;
    const char* words = luaL_checklstring(L, 2, &len);
    std::string in_words(words, len);
    lua_pushboolean(L, filter->Contains(in_words));
    return 1;
}

static int text_filter_replace(lua_State *L) {
    tinynet::text::TextFilter* filter = luaL_checktextfilter(L, 1);
    size_t len;
    const char* words = luaL_checklstring(L, 2, &len);
    if (len == 0) {
        lua_pushstring(L, "");
        return 1;
    }
    std::string in_words(words, len);
    const char* rep = luaL_optstring(L, 3, "");
    std::string in_rep(rep);
    std::string output;
    filter->Replace(in_words, in_rep, &output);
    lua_pushlstring(L, &output[0], output.size());
    return 1;
}

static const luaL_Reg meta_methods[] = {
    { "Init", text_filter_init},
    { "ContainsKey", text_filter_contains_key},
    { "Contains", text_filter_contains },
    { "Replace", text_filter_replace},
    {"__gc", text_filter_delete },
    {0, 0}
};
static const luaL_Reg methods[] = {
    {"new", text_filter_new },
    { 0, 0 }
};

LUALIB_API int luaopen_textfilter(lua_State *L) {
    using namespace tinynet::text;
    luaL_newmetatable(L, TEXT_FILTER_META);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    LUA_WRITE_ENUM(TRIE_FILTER);
    LUA_WRITE_ENUM(BLOOM_FILTER);
    LUA_WRITE_ENUM(DARTS_FILTER);
    LUA_WRITE_ENUM(RADIX_FILTER);
    return 1;
}
