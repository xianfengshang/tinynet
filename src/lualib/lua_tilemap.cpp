// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <memory>
#include <functional>
#include <algorithm>
#include "tilemap/tilemap.h"
#include "lua_tilemap.h"
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"
#include "lua_proto_types.h"

#define  TILEMAP_META_TABLE "tilemap_meta_table"

static tinynet::tilemaps::TileMap* luaL_checktilemap(lua_State *L, int idx) {
    return (tinynet::tilemaps::TileMap*)luaL_checkudata(L, idx, TILEMAP_META_TABLE);
}

static int tilemap_new(lua_State *L) {
    auto tilemap = lua_newuserdata(L, sizeof(tinynet::tilemaps::TileMap));
    new(tilemap) tinynet::tilemaps::TileMap();
    luaL_getmetatable(L, TILEMAP_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int tilemap_delete(lua_State *L) {
    auto tilemap = luaL_checktilemap(L, 1);
    tilemap->~TileMap();
    return 0;
}

static int tilemap_add(lua_State *L) {
    auto tilemap = luaL_checktilemap(L, 1);

    tinynet::tilemaps::EntryId id = luaL_checkinteger(L, 2);
    luaL_argcheck(L, lua_istable(L, 3), 3, "vector2 or vector3 expected!");
    luaL_argcheck(L, lua_istable(L, 4), 4, "vector2 or vector3 expected!");

    LuaState S{ L };

    tinynet::tilemaps::Box bound;

    lua_pushvalue(L, 3);
    S >> bound.min_corner();
    lua_pop(L, 1);

    S >> bound.max_corner();

    tilemap->Add(id, bound);
    return 0;
}

static int tilemap_remove(lua_State *L) {
    auto tilemap = luaL_checktilemap(L, 1);

    tinynet::tilemaps::EntryId id = luaL_checkinteger(L, 2);
    lua_pushboolean(L, tilemap->Remove(id));
    return 1;
}

static int tilemap_size(lua_State *L) {
    auto tilemap = luaL_checktilemap(L, 1);
    lua_pushinteger(L, static_cast<lua_Integer>(tilemap->Size()));
    return 1;
}

static int tilemap_query_bound(lua_State *L) {
    auto tilemap = luaL_checktilemap(L, 1);

    luaL_argcheck(L, lua_istable(L, 2), 2, "vector3 expected!");
    luaL_argcheck(L, lua_istable(L, 3), 3, "vector3 expected!");

    tinynet::tilemaps::Box bound;
    LuaState S{ L };
    lua_pushvalue(L, 2);
    S >> bound.min_corner();
    lua_pop(L, 1);
    S >> bound.max_corner();
    std::vector<tinynet::tilemaps::Entry> output;
    tilemap->QueryBound(bound, &output);

    std::vector<int64_t> entities(output.size());
    for (size_t i = 0; i < output.size(); ++i)
        entities[i] = output[i].second;
    S << entities;
    return 1;
}

static int tilemap_contains(lua_State *L) {
    auto tilemap = luaL_checktilemap(L, 1);

    tinynet::tilemaps::EntryId id = luaL_checkinteger(L, 2);
    lua_pushboolean(L, tilemap->Contains(id));
    return 1;
}

static const luaL_Reg meta_methods[] = {
    { "Add", tilemap_add},
    { "Update", tilemap_add},
    { "Remove", tilemap_remove},
    { "QueryBound", tilemap_query_bound},
    { "Size", tilemap_size},
    { "Contains", tilemap_contains},
    {"__gc", tilemap_delete },
    {0, 0}
};
static const luaL_Reg methods[] = {
    {"new", tilemap_new },
    { 0, 0 }
};

LUALIB_API int luaopen_tilemap(lua_State *L) {
    luaL_newmetatable(L, TILEMAP_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
