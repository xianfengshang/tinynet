// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <memory>
#include <functional>
#include <algorithm>
#include "lua_geo_types.h"
#include "lua_geo.h"
#include "geo/geo_service.h"
#include "lua_helper.h"
#include "lua_script.h"
#include "lua_compat.h"

#define  GEO_SERVICE_META_TABLE "geo_service_meta_table"

static tinynet::geo::Point* luaL_topoint(lua_State *L, int idx, tinynet::geo::Point* outpoint) {
    if (outpoint == nullptr) {
        return outpoint;
    }
    idx = lua_absindex(L, idx);

    LuaState S{ L };
    int n = lua_gettop(L) - idx + 1;
    if (n == 1) {
        if (lua_istable(L, idx)) {
            lua_pushvalue(L, idx);
            S >> *outpoint;
            lua_pop(L, 1);
        } else {
            luaL_error(L, "Bad geo point");
        }
    } else if (n == 2) {
        outpoint->set<0>(lua_tonumber(L, idx));
        outpoint->set<1>(lua_tonumber(L, idx + 1));
    }
    return outpoint;
}

struct GeometryReg {
    const char* name;
    unsigned long long hash;
    lua_CFunction contains;
    lua_CFunction centroid;
};

#define GEOMETRY_REG(NAME) {#NAME, StringUtils::Hash3(#NAME), NAME##_contains, NAME##_centroid}

static int Point_contains(lua_State *L) {
    tinynet::geojson::Point point {{0, 0}};
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> point;
    lua_pop(L, 1);

    tinynet::geo::Point pt;
    luaL_topoint(L, 2, &pt);
    bool ret = bg::within(pt, point.coordinates);
    lua_pushboolean(L, ret);
    return 1;
}

static int Point_centroid(lua_State *L) {
    tinynet::geojson::Point point {{0, 0}};
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> point;
    lua_pop(L, 1);

    S << point.coordinates;
    return 1;
}

static int MultiPoint_contains(lua_State *L) {
    tinynet::geojson::MultiPoint mpoint;
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> mpoint;
    lua_pop(L, 1);

    if (mpoint.coordinates.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }
    tinynet::geo::Point pt;
    luaL_topoint(L, 2, &pt);
    bool ret = bg::within(pt, mpoint.coordinates);
    lua_pushboolean(L, ret);
    return 1;
}

static int MultiPoint_centroid(lua_State *L) {
    return 0;
}

static int LineString_contains(lua_State *L) {
    tinynet::geojson::LineString linestring;
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> linestring;
    lua_pop(L, 1);
    if (linestring.coordinates.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }
    tinynet::geo::Point pt;
    luaL_topoint(L, 2, &pt);
    bool ret = bg::within(pt, linestring.coordinates);
    lua_pushboolean(L, ret);
    return 1;
}

static int LineString_centroid(lua_State *L) {
    return 0;
}

static int MultiLineString_contains(lua_State *L) {
    tinynet::geojson::LineString mlinestring;
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> mlinestring;
    lua_pop(L, 1);
    if (mlinestring.coordinates.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }
    tinynet::geo::Point pt;
    luaL_topoint(L, 2, &pt);
    bool ret = bg::within(pt, mlinestring.coordinates);
    lua_pushboolean(L, ret);
    return 1;
}

static int MultiLineString_centroid(lua_State *L) {
    return 0;
}

static int Polygon_contains(lua_State *L) {
    tinynet::geojson::Polygon polygon_info;
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> polygon_info;
    lua_pop(L, 1);
    tinynet::geo::Polygon polygon;
    if (polygon_info.coordinates.size() > 0) {
        polygon.outer() = polygon_info.coordinates[0];
    }
    if (polygon_info.coordinates.size() > 1) {
        polygon.inners().assign(polygon_info.coordinates.begin() + 1, polygon_info.coordinates.end());
    }

    tinynet::geo::Point pt;
    luaL_topoint(L, 2, &pt);
    bool ret = bg::within(pt, polygon);
    lua_pushboolean(L, ret);
    return 1;
}

static int Polygon_centroid(lua_State *L) {
    tinynet::geojson::Polygon polygon_info;
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> polygon_info;
    lua_pop(L, 1);
    tinynet::geo::Polygon polygon;
    if (polygon_info.coordinates.size() > 0) {
        polygon.outer() = polygon_info.coordinates[0];
    }
    if (polygon_info.coordinates.size() > 1) {
        polygon.inners().assign(polygon_info.coordinates.begin() + 1, polygon_info.coordinates.end());
    }

    tinynet::geo::Point point;
    bg::strategy::centroid::geo_average avg;
    bg::centroid(polygon, point, avg);
    S << point;
    return 1;
}

static int MultiPolygon_contains(lua_State *L) {
    tinynet::geojson::MultiPolygon multi_polygon_info;
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> multi_polygon_info;
    lua_pop(L, 1);
    tinynet::geo::MultiPolygon multi_polygon;
    for (auto& polygon_info : multi_polygon_info.coordinates) {
        tinynet::geo::Polygon polygon;
        if (polygon_info.size() > 0) {
            polygon.outer() = polygon_info[0];
        }
        if (polygon_info.size() > 1) {
            polygon.inners().assign(polygon_info.begin() + 1, polygon_info.end());
        }
        multi_polygon.emplace_back(std::move(polygon));
    }
    if (multi_polygon.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }
    tinynet::geo::Point pt;
    luaL_topoint(L, 2, &pt);
    bool ret = bg::within(pt, multi_polygon);
    lua_pushboolean(L, ret);
    return 1;
}

static int MultiPolygon_centroid(lua_State *L) {
    tinynet::geojson::MultiPolygon multi_polygon_info;
    LuaState S{ L };
    lua_pushvalue(L, 1);
    S >> multi_polygon_info;
    lua_pop(L, 1);
    tinynet::geo::MultiPolygon multi_polygon;
    for (auto& polygon_info : multi_polygon_info.coordinates) {
        tinynet::geo::Polygon polygon;
        if (polygon_info.size() > 0) {
            polygon.outer() = polygon_info[0];
        }
        if (polygon_info.size() > 1) {
            polygon.inners().assign(polygon_info.begin() + 1, polygon_info.end());
        }
        multi_polygon.emplace_back(std::move(polygon));
    }
    if (multi_polygon.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }

    tinynet::geo::Point point;
    bg::strategy::centroid::geo_average avg;
    bg::centroid(multi_polygon, point, avg);
    S << point;
    return 1;
}


GeometryReg geometry_regs[] = {
    GEOMETRY_REG(Point),
    GEOMETRY_REG(MultiPoint),
    GEOMETRY_REG(LineString),
    GEOMETRY_REG(MultiLineString),
    GEOMETRY_REG(Polygon),
    GEOMETRY_REG(MultiPolygon),
    {0, 0, 0, 0},
};

static int geo_contains(lua_State *L) {
    luaL_argcheck(L, lua_istable(L, 1), 1, "geo geometry expected!");
    LuaState S{ L };
    tinynet::geojson::Geometry geometry;
    lua_pushvalue(L, 1);
    S >> geometry;
    lua_pop(L, 1);
    auto hash = StringUtils::Hash3(geometry.type.c_str());
    for (GeometryReg* reg = geometry_regs; reg->name; reg++) {
        if (reg->hash == hash) {
            return reg->contains(L);
        }
    }
    return luaL_error(L, "geo geometry '%s' not supported", geometry.type.c_str());
}

static int geo_centroid(lua_State *L) {
    luaL_argcheck(L, lua_istable(L, 1), 1, "geo geometry expected!");
    LuaState S{ L };
    tinynet::geojson::Geometry geometry;
    lua_pushvalue(L, 1);
    S >> geometry;
    lua_pop(L, 1);
    auto hash = StringUtils::Hash3(geometry.type.c_str());
    for (GeometryReg* reg = geometry_regs; reg->name; reg++) {
        if (reg->hash == hash) {
            return reg->centroid(L);
        }
    }
    return luaL_error(L, "geo geometry '%s' not supported", geometry.type.c_str());
}

static tinynet::geo::GeoService* luaL_checkgeoservice(lua_State *L, int idx) {
    return (tinynet::geo::GeoService*)luaL_checkudata(L, idx, GEO_SERVICE_META_TABLE);
}

static int geo_service_new(lua_State *L) {
    auto geo = lua_newuserdata(L, sizeof(tinynet::geo::GeoService));
    new(geo) tinynet::geo::GeoService();
    luaL_getmetatable(L, GEO_SERVICE_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int geo_service_delete(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);
    geo->~GeoService();
    return 0;
}

static int geo_service_add(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);

    tinynet::geo::EntryId id = luaL_checkinteger(L, 2);

    tinynet::geo::Point pos;
    luaL_topoint(L, 3, &pos);

    geo->Add(id, pos);
    return 0;
}

static int geo_service_remove(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);

    tinynet::geo::EntryId id = luaL_checkinteger(L, 2);
    lua_pushboolean(L, geo->Remove(id));
    return 1;
}

static int geo_service_size(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);
    lua_pushinteger(L, static_cast<lua_Integer>(geo->Size()));
    return 1;
}

static int geo_service_query_radius(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);
    luaL_argcheck(L, lua_istable(L, 2), 2, "Geo point expected!");
    double distance = luaL_checknumber(L, 3);
    lua_pushvalue(L, 2);
    LuaState S{ L };
    tinynet::geo::Point center;
    S >> center;
    lua_pop(L, 1);

    std::vector<tinynet::geo::GeoSearchResult> results;
    geo->QueryRadius(center, distance, &results);
    S << results;
    return 1;
}

static int geo_service_query_nearest(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);
    tinynet::geo::EntryId entryId = static_cast<tinynet::geo::EntryId>(luaL_checknumber(L, 2));
    int n = luaL_optint(L, 3, 0);

    std::vector<tinynet::geo::GeoSearchResult> results;
    geo->QueryNearest(entryId, n, &results);
    LuaState S{ L };
    S << results;
    return 1;
}

static int geo_service_contains(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);

    tinynet::geo::EntryId id = luaL_checkinteger(L, 2);
    lua_pushboolean(L, geo->Contains(id));
    return 1;
}

static int geo_service_get_entry(lua_State *L) {
    auto geo = luaL_checkgeoservice(L, 1);

    tinynet::geo::EntryId id = luaL_checkinteger(L, 2);
    tinynet::geo::Coordinate coord;
    bool ret = geo->TryGetEntry(id, &coord);
    if (!ret) {
        return 0;
    }
    LuaState S{ L };
    S << coord;
    return 1;
}

static const luaL_Reg geo_service_meta_methods[] = {
    { "Add", geo_service_add},
    { "Remove", geo_service_remove},
    { "QueryRadius", geo_service_query_radius},
    { "QueryNearest", geo_service_query_nearest},
    { "Size", geo_service_size},
    { "Contains", geo_service_contains},
    { "GetEntry", geo_service_get_entry},
    {"__gc", geo_service_delete },
    {0, 0}
};

static const luaL_Reg methods[] = {
    {"contains", geo_contains },
    {"centroid", geo_centroid},
    {"new", geo_service_new},
    { 0, 0 }
};

LUALIB_API int luaopen_geo(lua_State *L) {
    luaL_newmetatable(L, GEO_SERVICE_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, geo_service_meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
