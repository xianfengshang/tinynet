// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_helper.h"
#include "logging/logging.h"
#include "lua_compat.h"
#include "lua_proto_types.h"
#include "base/error_code.h"
#include "tdc/tdc_service.h"
#include "cluster/cluster_service.h"
#include "lua_script.h"
#include "app/app_container.h"

using namespace tinynet;

static bool cluster_receive_message_callback(lua_State* L, tdc::TdcService* tdc, bool bytes_as_string, const std::string &msg_body) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, (void *)tdc);
    if (!lua_isfunction(L, -1)) {
        return false;
    }
    if (bytes_as_string) {
        lua_pushlstring(L, msg_body.c_str(), msg_body.size());
    } else {
        //lua_pushbytes(L, &body);
        lua_pushandswapbytes(L, const_cast<std::string*>(&msg_body)); //Warning: Never use body again
    }
    if (luaL_pcall(L, 1, 1) != LUA_OK) {
        return false;
    }
    bool result = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result;
}

static int cluster_start(lua_State* L) {
    auto app = lua_getapp(L);
    auto cluster = app->get<tinynet::cluster::ClusterService>();
    lua_State* LL = app->get<lua_State>();
    const char* id = luaL_checkstring(L, 1);
    luaL_argcheck(L, lua_type(L, 2) == LUA_TTABLE, 2, "table expected!");
    lua_pushvalue(L, 2);
    tinynet::cluster::ClusterOptions opts;
    LuaState S{ L };
    S >> opts;
    lua_pop(L, 1);

    int err = cluster->Start(id, opts);
    if (err != ERROR_OK) {
        lua_pushstring(L, tinynet_strerror(err));
        return 1;
    }
    auto tdc = cluster->get_tdc(id);
    if (tdc && lua_isfunction(L, 3)) {
        lua_pushvalue(L, 3);
        lua_rawsetp(L, LUA_REGISTRYINDEX, (void*)tdc.get());
        auto callback = std::bind(cluster_receive_message_callback, LL, tdc.get(), opts.bytesAsString, std::placeholders::_1);
        tdc->set_receive_msg_callback(callback);
    }
    return 0;
}

static int cluster_stop(lua_State* L) {
    auto app = lua_getapp(L);
    auto cluster = app->get<tinynet::cluster::ClusterService>();
    for (auto& entry : cluster->tdc_map()) {
        lua_pushnil(L);
        lua_rawsetp(L, LUA_REGISTRYINDEX, (void*)entry.second.get());
    }
    cluster->Stop();
    return 0;
}


static void cluster_send_message_callback(lua_State* L, int nref, int32_t err) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, nref);
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    if (!lua_isfunction(L, -1)) {
        log_warning("Timer can not found timeout callback function!");
        lua_pop(L, 1);
        return;
    }
    int nargs = 0;
    if (err) {
        lua_pushstring(L, tinynet_strerror(err));
        nargs = 1;
    }
    luaL_pcall(L, nargs, 0);
}

static int cluster_send_message(lua_State *L) {
    auto app = lua_getapp(L);
    auto cluster = app->get<cluster::ClusterService>();
    lua_State* LL = app->get<lua_State>();
    if (cluster->tdc_size() == 0) {
        return luaL_error(L, "Please init cluster node first!");
    }
    const char* id = luaL_checkstring(L, 1);

    luaL_argcheck(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function expected!");
    lua_pushvalue(L, 3);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    auto callback = std::bind(cluster_send_message_callback, LL, nref, std::placeholders::_1);

    int type = lua_type(L, 2);
    switch (type) {
    case LUA_TSTRING: {
        size_t len;
        const char* data = luaL_checklstring(L, 2, &len);
        cluster->SendMsg(id, data, len, callback);
        return 0;
    }
    case LUA_TUSERDATA: {
        auto data = luaL_checkbytes(L, 2);
        cluster->SendMsg(id, *data, callback);
        return 0;
    }
    default:
        luaL_unref(L, LUA_REGISTRYINDEX, nref);
        return luaL_argerror(L, 2, "string or bytes expected");
    }
}


static void tns_callback(lua_State* L, int nref, const tinynet::naming::NamingReply& reply) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, nref);
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    if (!lua_isfunction(L, -1)) {
        log_warning("Tns reply can not found timeout callback function!");
        lua_pop(L, 1);
        return;
    }
    int nargs = 0;
    LuaState S{ L };
    switch (reply.type) {
    case tinynet::naming::NamingReplyType::GET:
        nargs = 1;
        if (reply.err == 0) {
            S << reply.value;
        } else {
            lua_pushnil(L);
        }
        break;
    case tinynet::naming::NamingReplyType::PUT:
        break;
    case tinynet::naming::NamingReplyType::DEL:
        break;
    case tinynet::naming::NamingReplyType::KEYS:
        nargs = 1;
        if (reply.err == 0) {
            S << reply.keys;
        } else {
            lua_pushnil(L);
        }
        break;
    default:
        break;
    }
    if (reply.err != 0) {
        nargs++;
        lua_pushstring(L, tinynet_strerror(reply.err));
    }
    luaL_pcall(L, nargs, 0);
}

static int tns_get(lua_State *L) {
    auto app = lua_getapp(L);
    auto cluster = app->get<cluster::ClusterService>();
    lua_State* LL = app->get<lua_State>();
    if (cluster->tdc_size() == 0) {
        return luaL_error(L, "Please init cluster node first!");
    }
    const char* key = luaL_checkstring(L, 1);
    luaL_argcheck(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "function expected");
    lua_pushvalue(L, 2);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    auto callback = std::bind(tns_callback, LL, nref, std::placeholders::_1);
    int err = cluster->Get(key, callback);
    if (err == ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int tns_put(lua_State *L) {
    auto app = lua_getapp(L);
    auto cluster = app->get<cluster::ClusterService>();
    lua_State* LL = app->get<lua_State>();
    if (cluster->tdc_size() == 0) {
        return luaL_error(L, "Please init cluster node first!");
    }
    const char* key = luaL_checkstring(L, 1);
    const char* value = luaL_checkstring(L, 2);
    uint32_t timeout = static_cast<uint32_t>(luaL_checkint(L, 3));
    luaL_argcheck(L, lua_type(L, 4) == LUA_TFUNCTION, 4, "function expected");
    lua_pushvalue(L, 4);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);

    auto callback = std::bind(tns_callback, LL, nref, std::placeholders::_1);
    int err = cluster->Put(key, value, timeout, callback);
    if (err == ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int tns_delete(lua_State *L) {
    auto app = lua_getapp(L);
    auto cluster = app->get<cluster::ClusterService>();
    lua_State* LL = app->get<lua_State>();
    if (cluster->tdc_size() == 0) {
        return luaL_error(L, "Please init cluster node first!");
    }
    const char* key = luaL_checkstring(L, 1);
    luaL_argcheck(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "function expected");
    lua_pushvalue(L, 2);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);

    auto callback = std::bind(tns_callback, LL, nref, std::placeholders::_1);
    int err = cluster->Delete(key, callback);
    if (err == ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int tns_keys(lua_State *L) {
    auto app = lua_getapp(L);
    auto cluster = app->get<cluster::ClusterService>();
    lua_State* LL = app->get<lua_State>();
    if (cluster->tdc_size() == 0) {
        return luaL_error(L, "Please init cluster node first!");
    }
    const char* key = luaL_checkstring(L, 1);
    luaL_argcheck(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "function expected");
    lua_pushvalue(L, 2);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);

    auto callback = std::bind(tns_callback, LL, nref, std::placeholders::_1);
    int err = cluster->Keys(key, callback);
    if (err == ERROR_OK) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static const luaL_Reg methods[] = {
    { "start", cluster_start},
    { "stop", cluster_stop},
    { "send_message", cluster_send_message },
    { "get", tns_get},
    { "put", tns_put},
    { "delete", tns_delete},
    { "keys", tns_keys},
    { 0, 0 }
};

LUALIB_API int luaopen_cluster(lua_State *L) {
    luaL_newlib(L, methods);
    return 1;
}
