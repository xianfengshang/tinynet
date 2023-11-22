// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "geo/geo_types.h"
#include "geo/geojson_types.h"
#include "geo/geo_service.h"
#include "lua_types.h"

inline const LuaState& operator >> (const LuaState& L, tinynet::geo::Point& o) {
    std::tuple<double, double> point;
    L >> point;
    o.set<0>(std::get<0>(point));
    o.set<1>(std::get<1>(point));
    return L;
}

inline LuaState& operator << (LuaState& L, tinynet::geo::Point & o) {
    std::tuple<double, double> point{ o.get<0>(), o.get<1>() };
    L << point;
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::geojson::Geometry& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(type);
    LUA_READ_END();
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::geojson::Point& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(coordinates);
    LUA_READ_END();
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::geojson::MultiPoint& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(coordinates);
    LUA_READ_END();
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::geojson::LineString& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(coordinates);
    LUA_READ_END();
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::geojson::MultiLineString& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(coordinates);
    LUA_READ_END();
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::geojson::Polygon& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(coordinates);
    LUA_READ_END();
    return L;
}

inline const LuaState& operator >> (const LuaState& L, tinynet::geojson::MultiPolygon& o) {
    LUA_READ_BEGIN();
    LUA_READ_FIELD(coordinates);
    LUA_READ_END();
    return L;
}

inline LuaState& operator << (LuaState& L, const tinynet::geo::GeoSearchResult & o) {
    LUA_WRITE_BEGIN();
    LUA_WRITE_FIELD(entryId);
    LUA_WRITE_FIELD(distance);
    LUA_WRITE_END();
    return L;
}

template<>
inline LuaState& operator << (LuaState& L, const std::vector<tinynet::geo::GeoSearchResult>& o) {
    lua_newtable(L.L);
    for (size_t i = 0; i < o.size(); ++i) {
        L << o[i];
        lua_rawseti(L.L, -2, static_cast<int>(i + 1));
    }
    return L;
}