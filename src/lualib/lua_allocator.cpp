// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_compat.h"
#include "logging/logging.h"
#include <vector>
#include <string>
#include "lua_types.h"
#include "lua_common_types.h"
#include "lua_proto_types.h"
#include "base/allocator.h"
#if defined USE_TCMALLOC
#include "gperftools/malloc_extension.h"

static int lua_tcmalloc_getmemoryreleaserate(lua_State *L) {
    lua_pushnumber(L, MallocExtension::instance()->GetMemoryReleaseRate());
    return 1;
}

static int lua_tcmalloc_setmemoryreleaserate(lua_State *L) {
    double  value = luaL_checknumber(L, 1);
    MallocExtension::instance()->SetMemoryReleaseRate(value);
    return 0;
}

static int lua_tcmalloc_releasefreememory(lua_State *L) {
    MallocExtension::instance()->ReleaseFreeMemory();
    return 0;
}

static int lua_tcmalloc_releasetosystem(lua_State *L) {
    size_t value = luaL_checkinteger(L, 1);
    MallocExtension::instance()->ReleaseToSystem(value);
    return 0;
}

static int lua_tcmalloc_getstats(lua_State *L) {
    std::vector<char> buf; //Avoid _chkstk crash
    buf.resize(1024 * 1024);
    MallocExtension::instance()->GetStats(&buf[0], (int)buf.size());
    lua_pushstring(L, buf.data()); // Now buf is a null-terminated string
    return 1;
}
static int lua_tcmalloc_getnumericproperty(lua_State *L) {
    size_t value = 0;
    const char* name = luaL_checkstring(L, 1);
    MallocExtension::instance()->GetNumericProperty(name, &value);
    lua_pushnumber(L, static_cast<lua_Number>(value));
    return 1;
}

static int lua_tcmalloc_setnumericproperty(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);
    size_t value = luaL_checkinteger(L, 2);
    bool result = MallocExtension::instance()->SetNumericProperty(name, value);
    lua_pushboolean(L, result);
    return 1;
}

static const luaL_Reg methods[] = {
    { "GetMemoryReleaseRate", lua_tcmalloc_getmemoryreleaserate },
    { "SetMemoryReleaseRate", lua_tcmalloc_setmemoryreleaserate },
    { "ReleaseFreeMemory", lua_tcmalloc_releasefreememory },
    { "ReleaseToSystem", lua_tcmalloc_releasetosystem },
    { "GetStats", lua_tcmalloc_getstats},
    { "GetNumericProperty", lua_tcmalloc_getnumericproperty},
    { "SetNumericProperty", lua_tcmalloc_setnumericproperty},
    {0, 0}
};

LUALIB_API int luaopen_allocator(lua_State *L) {
    lua_newtable(L);
    luaL_newlib(L, methods);
    lua_setfield(L, -2, "tcmalloc");
    return 1;
}
#elif defined USE_MIMALLOC
int  lua_mi_option_is_enabled(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    lua_pushboolean(L, mi_option_is_enabled(option));
    return 1;
}

int  lua_mi_option_enable(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    mi_option_enable(option);
    return 0;
}
int  lua_mi_option_disable(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    mi_option_disable(option);
    return 0;
}

int  lua_mi_option_set_enabled(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    bool enable = lua_toboolean(L, 2);
    mi_option_set_enabled(option, enable);
    return 0;
}

int  lua_mi_option_set_enabled_default(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    bool enable = lua_toboolean(L, 2);
    mi_option_set_enabled_default(option, enable);
    return 0;
}

int  lua_mi_option_get(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    lua_pushinteger(L, mi_option_get(option));
    return 1;
}

int lua_mi_option_set(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    lua_Integer value = luaL_checkinteger(L, 2);
    mi_option_set(option, static_cast<long>(value));
    return 0;
}

int  lua_mi_option_set_default(lua_State *L) {
    mi_option_t option = (mi_option_t)luaL_checkint(L, 1);
    lua_Integer value = luaL_checkinteger(L, 2);
    mi_option_set_default(option, static_cast<long>(value));
    return 0;
}

int lua_mi_collect(lua_State *L) {
    bool force = lua_toboolean(L, 1);
    mi_collect(force);
    return 0;
}

int lua_mi_stats_reset(lua_State *L) {
    mi_stats_reset();
    return 0;
}

int lua_mi_stats_merge(lua_State *L) {
    mi_stats_merge();
    return 0;
}

static void output_fun(const char* msg, void* arg) {
    luaL_Buffer* b= (luaL_Buffer*)arg;
    luaL_addstring(b, msg);
}

int lua_mi_stats_print(lua_State *L) {
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    mi_stats_print_out(output_fun, &b);
    luaL_pushresult(&b);
    return 1;
}

static const luaL_Reg methods[] = {
    { "mi_option_is_enabled", lua_mi_option_is_enabled },
    { "mi_option_enable", lua_mi_option_enable },
    { "mi_option_disable", lua_mi_option_disable },
    { "mi_option_set_enabled", lua_mi_option_set_enabled },
    { "mi_option_set_enabled_default", lua_mi_option_set_enabled},
    { "mi_option_get", lua_mi_option_get},
    { "mi_option_set", lua_mi_option_set},
    { "mi_option_set_default", lua_mi_option_set_default},
    { "mi_collect", lua_mi_collect},
    { "mi_stats_reset", lua_mi_stats_reset},
    { "mi_stats_merge", lua_mi_stats_merge},
    { "mi_stats_print", lua_mi_stats_print},
    {0, 0}
};

LUALIB_API int luaopen_allocator(lua_State *L) {
    lua_newtable(L);

    luaL_newlib(L, methods);

    //push xml_node_type enum
    lua_pushstring(L, "mi_option_t");
    lua_newtable(L);
    LUA_WRITE_ENUM(mi_option_show_errors);
    LUA_WRITE_ENUM(mi_option_show_stats);
    LUA_WRITE_ENUM(mi_option_verbose);
    // the following options are experimental
    LUA_WRITE_ENUM(mi_option_eager_commit);
    LUA_WRITE_ENUM(mi_option_eager_region_commit);
    LUA_WRITE_ENUM(mi_option_reset_decommits);
    LUA_WRITE_ENUM(mi_option_large_os_pages);         // implies eager commit
    LUA_WRITE_ENUM(mi_option_reserve_huge_os_pages);
    LUA_WRITE_ENUM(mi_option_reserve_huge_os_pages_at);
    LUA_WRITE_ENUM(mi_option_reserve_os_memory);
    LUA_WRITE_ENUM(mi_option_segment_cache);
    LUA_WRITE_ENUM(mi_option_page_reset);
    LUA_WRITE_ENUM(mi_option_abandoned_page_reset);
    LUA_WRITE_ENUM(mi_option_segment_reset);
    LUA_WRITE_ENUM(mi_option_eager_commit_delay);
    LUA_WRITE_ENUM(mi_option_reset_delay);
    LUA_WRITE_ENUM(mi_option_use_numa_nodes);
    LUA_WRITE_ENUM(mi_option_limit_os_alloc);
    LUA_WRITE_ENUM(mi_option_os_tag);
    LUA_WRITE_ENUM(mi_option_max_errors);
    LUA_WRITE_ENUM(mi_option_max_warnings);
    lua_rawset(L, -3);

    lua_setfield(L, -2, "mimalloc");
    return 1;
}
#elif defined USE_JEMALLOC


static void write_fun(void* arg, const char* msg) {
    luaL_Buffer* b = (luaL_Buffer*)arg;
    luaL_addstring(b, msg);
}

int lua_malloc_stats_print(lua_State *L) {
    const char* opts = luaL_optstring(L, 1, NULL);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    je_malloc_stats_print(write_fun, &b, opts);
    luaL_pushresult(&b);
    return 1;
}

int lua_memory_stats(lua_State *L) {
    tinynet::lua::MemoryStats stats;
    size_t sz = sizeof(size_t);
    size_t epoch = 1;
    je_mallctl("epoch", &epoch, &sz, &epoch, sz);
    je_mallctl("stats.allocated", &stats.allocated, &sz, NULL, 0);
    je_mallctl("stats.active", &stats.active, &sz, NULL, 0);
    je_mallctl("stats.resident", &stats.resident, &sz, NULL, 0);
    je_mallctl("stats.mapped", &stats.mapped, &sz, NULL, 0);
    je_mallctl("stats.retained", &stats.retained, &sz, NULL, 0);
    LuaState S{ L };
    S << stats;
    return 1;
}

int lua_memory_purge(lua_State  *L) {
    unsigned narenas = 0;
    size_t sz = sizeof(narenas);
    if (!je_mallctl("arenas.narenas", &narenas, &sz, NULL, 0)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "arena.%d.purge", narenas);
        je_mallctl(buf, NULL, 0, NULL, 0);
    }
    return 0;
}

static const luaL_Reg methods[] = {
    {"malloc_stats_print", lua_malloc_stats_print},
    {"memory_stats", lua_memory_stats},
    {"memory_purge", lua_memory_purge},
    {0, 0}
};

LUALIB_API int luaopen_allocator(lua_State *L) {
    lua_newtable(L);

    luaL_newlib(L, methods);

    lua_setfield(L, -2, "jemalloc");
    return 1;
}
#else
LUALIB_API int luaopen_allocator(lua_State *L) {
    lua_newtable(L);
    return 1;
}
#endif