// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <memory>
#include <functional>
#include <algorithm>
#include "aoi/aoi_service.h"
#include "lua_aoi.h"
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"
#include "lua_proto_types.h"

#define  AOI_META_TABLE "aoi_meta_table"

static tinynet::aoi::AoiService* luaL_checkaoi(lua_State *L, int idx) {
    return (tinynet::aoi::AoiService*)luaL_checkudata(L, idx, AOI_META_TABLE);
}

static int aoi_new(lua_State *L) {
    auto aoi = lua_newuserdata(L, sizeof(tinynet::aoi::AoiService));
    new(aoi) tinynet::aoi::AoiService();
    luaL_getmetatable(L, AOI_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int aoi_delete(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);
    aoi->~AoiService();
    return 0;
}

static int aoi_add(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);

    tinynet::aoi::EntryId id = luaL_checkinteger(L, 2);
    luaL_argcheck(L, lua_istable(L, 3), 3, "vector2 or vector3 expected!");
    luaL_argcheck(L, lua_istable(L, 4), 4, "vector2 or vector3 expected!");

    LuaState S{ L };

    tinynet::aoi::Coordinate pos, size;

    lua_pushvalue(L, 3);
    S >> pos;
    lua_pop(L, 1);

    S >> size;

    aoi->Add(id, pos, size);
    return 0;
}

static int aoi_remove(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);

    tinynet::aoi::EntryId id = luaL_checkinteger(L, 2);
    lua_pushboolean(L, aoi->Remove(id));
    return 1;
}

static int aoi_size(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);
    lua_pushinteger(L, static_cast<lua_Integer>(aoi->Size()));
    return 1;
}

static void query_radius(tinynet::aoi::AoiService* aoi, const tinynet::Vector3& pos, float radius, std::vector<int64_t>* out) {
    tinynet::Vector3 size{ radius * 2, radius * 2, 0 };
    tinynet::Bounds bound(pos, size);
    std::vector<tinynet::aoi::Entry> output;
    aoi->QueryBound(bound, &output);

    for (auto& entry : output) {
        if (bg::distance(pos, entry.second) <= static_cast<double>(radius))
            out->push_back(entry.first);
    }
}

static int aoi_query_radius(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);

    luaL_argcheck(L, lua_istable(L, 2), 2, "vector2 or vector3 expected!");
    float radius = static_cast<float>(luaL_checknumber(L, 3));
    lua_pushvalue(L, 2);
    LuaState S{ L };
    tinynet::Vector3 center;
    S >> center;
    lua_pop(L, 1);

    std::vector<int64_t> entities;
    query_radius(aoi, center, radius, &entities);
    S << entities;
    return 1;
}

static int aoi_query_bound(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);

    luaL_argcheck(L, lua_istable(L, 2), 2, "vector2 or vector3 expected!");
    luaL_argcheck(L, lua_istable(L, 3), 3, "vector2 or vector3 expected!");

    tinynet::Vector3 center, size;
    LuaState S{ L };
    S >> size;
    lua_pushvalue(L, 2);
    S >> center;
    lua_pop(L, 1);

    tinynet::Bounds bound(center, size);
    std::vector<tinynet::aoi::Entry> output;
    aoi->QueryBound(bound, &output);

    std::vector<int64_t> entities(output.size());
    for (size_t i = 0; i < output.size(); ++i)
        entities[i] = output[i].first;
    S << entities;
    return 1;
}

static int aoi_query_nearby(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);
    tinynet::aoi::EntryId id = luaL_checkinteger(L, 2);
    float distance = static_cast<float>(luaL_checknumber(L, 3));
    std::vector<int64_t> entities;
    tinynet::Vector3 center;
    if (aoi->TryGetEntry(id, &center, nullptr)) {
        tinynet::Vector3 size{ distance * 2, distance * 2, 0 };
        tinynet::Bounds bound(center, size);
        std::vector<tinynet::aoi::Entry> output;
        aoi->QueryBound(bound, &output);
        for (auto& entry : output) {
            if (entry.first != id && bg::distance(center, entry.second) <= static_cast<double>(distance))
                entities.push_back(entry.first);
        }
    }
    LuaState S{ L };
    S << entities;
    return 1;
}

static int aoi_diff(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);
    luaL_argcheck(L, lua_istable(L, 2), 2, "vector2 or vector3 expected!");
    luaL_argcheck(L, lua_istable(L, 3), 3, "vector2 or vector3 expected!");
    float radius = static_cast<float>(luaL_checknumber(L, 4));

    LuaState S{ L };
    lua_pushvalue(L, 2);
    tinynet::Vector3 pos_left;
    S >> pos_left;
    lua_pop(L, 1);

    lua_pushvalue(L, 3);
    tinynet::Vector3 pos_right;
    S >> pos_right;
    lua_pop(L, 1);

    std::vector<int64_t> left, right, diff_left, diff_right;
    query_radius(aoi, pos_left, radius, &left);
    query_radius(aoi, pos_right, radius, &right);

    std::set_difference(left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(diff_left));
    std::set_difference(right.begin(), right.end(), left.begin(), left.end(), std::back_inserter(diff_right));
    S << diff_left;
    S << diff_right;
    return 2;
}


static int aoi_contains(lua_State *L) {
    auto aoi = luaL_checkaoi(L, 1);

    tinynet::aoi::EntryId id = luaL_checkinteger(L, 2);
    lua_pushboolean(L, aoi->Contains(id));
    return 1;
}

static const luaL_Reg meta_methods[] = {
    { "Add", aoi_add},
    { "Update", aoi_add},
    { "Remove", aoi_remove},
    { "QueryBound", aoi_query_bound},
    { "QueryRadius", aoi_query_radius},
    { "QueryNearby", aoi_query_nearby},
    { "Size", aoi_size},
    { "Contains", aoi_contains},
    { "Diff", aoi_diff},
    {"__gc", aoi_delete },
    {0, 0}
};
static const luaL_Reg methods[] = {
    {"new", aoi_new },
    { 0, 0 }
};

LUALIB_API int luaopen_aoi(lua_State *L) {
    luaL_newmetatable(L, AOI_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
