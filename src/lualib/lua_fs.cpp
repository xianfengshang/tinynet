// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_fs.h"
#include "base/io_buffer.h"
#include "lua_helper.h"
#include "lua_script.h"
#include "app/app_container.h"
#include "tfs/tfs_service.h"
#include "lua_types.h"
#include "lua_proto_types.h"

static inline int readable(tinynet::lua::LuaScript* ctx, const char *filename) {
    return ctx->FileExists(filename);
}

static const char *pushnexttemplate(lua_State *L, const char *path) {
    const char *l;
    while (*path == ';') path++;  /* skip separators */
    if (*path == '\0') return NULL;  /* no more templates */
    l = strchr(path, ';');  /* find next separator */
    if (l == NULL) l = path + strlen(path);
    lua_pushlstring(L, path, l - path);  /* template */
    return l;
}

static const char *findfile(lua_State *L, const char *name,
                            const char *pname) {
    auto app = lua_getapp(L);
    const char *path;
    name = luaL_gsub(L, name, ".", LUA_DIRSEP);
    lua_getglobal(L, "package");
    lua_getfield(L, -1, pname);
    lua_remove(L, -2);
    path = lua_tostring(L, -1);
    if (path == NULL)
        luaL_error(L, "\"package.%s\" must be a string", pname);
    lua_pushliteral(L, "");  /* error accumulator */
    while ((path = pushnexttemplate(L, path)) != NULL) {
        const char *filename;
        filename = luaL_gsub(L, lua_tostring(L, -1), LUA_PATH_MARK, name);
        lua_remove(L, -2);  /* remove path template */
        if (readable(app->get<tinynet::lua::LuaScript>(), filename))  /* does file exist and is readable? */
            return filename;  /* return that file name */
        lua_pushfstring(L, "\n\tno file \"%s\"", filename);
        lua_remove(L, -2);  /* remove file name */
        lua_concat(L, 2);  /* add entry to possible error message */
    }
    return NULL;  /* not found */
}


static void loaderror(lua_State *L, const char *filename) {
    luaL_error(L, "error loading module \" %s \" from file \" %s \":\n\t%s",
               lua_tostring(L, 1), filename, lua_tostring(L, -1));
}


static int loader_zip(lua_State *L) {
    const char *filename;
    auto app = lua_getapp (L);
    const char *name = luaL_checkstring(L, 1);
    filename = findfile(L, name, "path");
    if (filename == NULL) return 1;  /* library not found in this path */
    std::string filecontent;
    if (!app->get<tinynet::tfs::TfsService>()->LoadFile(filename, &filecontent)) {
        lua_pushstring(L, "file not found");
        return 1;
    }
    if (luaL_loadbuffer(L, &filecontent[0], filecontent.size(), filename) != 0)
        loaderror(L, filename);
#if (LUA_VERSION_NUM <= 501)
    return 1;  /* library loaded successfully */
#else
    lua_pushstring(L, filename);
    return 2;
#endif
}

static int lua_fs_exists(lua_State *L) {
    auto app = lua_getapp(L);
    const char* path = luaL_checkstring(L, 1);
    lua_pushboolean(L, app->get<tinynet::tfs::TfsService>()->Exists(path));
    return 1;
}

static int lua_fs_readfile(lua_State *L) {
    auto app = lua_getapp(L);
    const char* path = luaL_checkstring(L, 1);
    tinynet::IOBuffer io_buf;
    bool res = app->get<tinynet::tfs::TfsService>()->LoadFile(path, &io_buf);
    if (!res) {
        lua_pushnil(L);
        lua_pushstring(L, "Unknown error");
        return 2;
    }
    lua_pushlstring(L, io_buf.begin(), io_buf.size());
    return 1;
}

static int lua_fs_readdir(lua_State *L) {
    auto app = lua_getapp(L);
    const char* path = luaL_checkstring(L, 1);
    std::vector<tinynet::tfs::DirectoryEntry> results;
    bool res = app->get<tinynet::tfs::TfsService>()->Readdir(path, results);
    if (!res) {
        lua_pushnil(L);
        lua_pushstring(L, "Unknown error");
        return 2;
    }
    LuaState S{ L };
    S << results;
    return 1;
}

static int lua_fs_clearcache(lua_State *L) {
    auto app = lua_getapp(L);
    app->get<tinynet::tfs::TfsService>()->ClearCache();
    return 0;
}

static const luaL_Reg methods[] = {
    { "exists", lua_fs_exists},
    { "readfile", lua_fs_readfile},
    { "readdir", lua_fs_readdir},
    { "clearcache", lua_fs_clearcache},
    { 0, 0}
};

LUALIB_API int luaopen_fs(lua_State *L) {
    //register zip archive loader
    lua_getglobal(L, "package");
#if (LUA_VERSION_NUM <= 501)
    lua_getfield(L, -1, "loaders");
    if (lua_istable(L, -1)) {
        lua_pushcfunction(L, loader_zip);
        lua_rawseti(L, -2, (int)lua_objlen(L, -2) + 1);
    }
#else
    lua_getfield(L, -1, "searchers");
    if (lua_istable(L, -1)) {
        lua_pushcfunction(L, loader_zip);
        lua_rawseti(L, -2, (int)lua_rawlen(L, -2) + 1);
    }
#endif
    lua_pop(L, 2);
    luaL_newlib(L, methods);
    return 1;
}
