// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_logger.h"
#include "lua_compat.h"
#include "logging/logging.h"
#include "base/runtime_logger.h"
#include "util/string_utils.h"
#include "lua_types.h"
#include <vector>
#include <string>

using namespace tinynet::logging;

static int lua_log(lua_State *L) {
    size_t len;
    const char* file = luaL_optstring(L, 1, "unknown");
    int line = luaL_optint(L, 2, 0);
    int level = luaL_optint(L, 3, LOG_LEVEL_INFO);
    const char* msg = luaL_checklstring(L, 4, &len);
    g_Logger->Log(file, line, level, msg, len);
    return 0;
}

static int lua_set_logtostderr(lua_State *L) {
    bool value = (bool)lua_toboolean(L, 1);
    g_Logger->set_logtostderr(value);
    return 0;
}

static int lua_set_logbufsecs(lua_State *L) {
    int value = luaL_checkint(L, 1);
    g_Logger->set_logbufsecs(value);
    return 0;
}


static int lua_set_minloglevel(lua_State *L) {
    int value = luaL_checkint(L, 1);
    g_Logger->set_minloglevel(value);
    return 0;
}

static int lua_set_max_log_size(lua_State *L) {
    int value = luaL_checkint(L, 1);
    g_Logger->set_max_log_size(value);
    return 0;
}

static int lua_log_flush(lua_State *L) {
    g_Logger->Flush();
    return 0;
}

static int lua_runtime_error(lua_State *L) {
    const char* msg = luaL_checkstring(L, 1);
    int stackLevel = luaL_optint(L, 2, 1);

    lua_Debug debug;
    lua_getstack(L, stackLevel, &debug);
    lua_getinfo(L, "Sln", &debug);

    g_RuntimeLogger->Log(debug.source, debug.currentline, msg);
    return 0;
}

static int lua_get_pipe_name(lua_State *L) {
    lua_pushstring(L, g_Logger->get_pipe_name().c_str());
    return 1;
}

static int lua_write_file_log(lua_State *L) {
    const char* basename = luaL_checkstring(L, 1);
    size_t msgLen;
    const char* msg = luaL_checklstring(L, 2, &msgLen);

    g_Logger->WriteFileLog(basename, msg, msgLen);
    return 0;
}

static int lua_flush_file_log(lua_State *L) {
    const char* basename = luaL_checkstring(L, 1);
    g_Logger->FlushFileLog(basename);
    return 0;
}

static const luaL_Reg methods[] = {
    { "log", lua_log},
    { "set_logtostderr", lua_set_logtostderr},
    { "set_logbufsecs", lua_set_logbufsecs},
    { "set_minloglevel", lua_set_minloglevel},
    { "set_max_log_size", lua_set_max_log_size},
    { "flush", lua_log_flush},
    { "runtime_error", lua_runtime_error},
    { "get_pipe_name", lua_get_pipe_name},
    { "write_file_log", lua_write_file_log},
    { "flush_file_log", lua_flush_file_log},
    {0, 0}
};

LUALIB_API int luaopen_logger(lua_State *L) {
    luaL_newlib(L, methods);
    LUA_WRITE_ENUM(LOG_LEVEL_DEBUG);
    LUA_WRITE_ENUM(LOG_LEVEL_INFO);
    LUA_WRITE_ENUM(LOG_LEVEL_WARN);
    LUA_WRITE_ENUM(LOG_LEVEL_ERROR);
    LUA_WRITE_ENUM(LOG_LEVEL_FATAL);
    return 1;
}
