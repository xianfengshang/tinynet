// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_http.h"
#include "lua_helper.h"
#include "logging/logging.h"
#include "lua_compat.h"
#include "lua_proto_types.h"
#include "net/http/http_server.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"
#include "util/net_utils.h"
#include "base/error_code.h"
#include "app/app_container.h"
#include "lua_common_types.h"

#define  HTTP_SERVER_META_TABLE  "http_server_meta_table"

using namespace tinynet;

class LuaHttpServer {
  public:
    LuaHttpServer(app::AppContainer* app) :
        app_(app),
        nref_(LUA_REFNIL) {
    }
    ~LuaHttpServer() {
        set_event_callback(LUA_REFNIL);
    }
  public:
    LuaHttpServer(const LuaHttpServer&) = delete;
    LuaHttpServer(LuaHttpServer&&) = delete;
    LuaHttpServer& operator=(const LuaHttpServer&) = delete;
  public:
    http::HttpServer* get_server() { return server_.get(); }

    void set_event_callback(int nref) {
        if (nref_ != LUA_REFNIL && nref_ != nref) {
            lua_State *L = app_->get<lua_State>();
            luaL_unref(L, LUA_REGISTRYINDEX, nref_);
        }
        nref_ = nref;
    }
  public:
    int Start(net::ServerOptions& opts, int nref) {
        int err;
        if (server_) return ERROR_SERVER_STARTED;
        server_.reset(new (std::nothrow) http::HttpServer(app_->event_loop()));
        if (!server_) return ERROR_OS_OOM;
        if (!opts.listen_url.empty()) {
            if (!UriUtils::parse_address(opts.listen_url, &opts.listen_ip, &opts.listen_port)) {
                return ERROR_URI_UNRECOGNIZED;
            }
        }
        if ((err = server_->Start(opts))) {
            server_.reset();
            return err;
        }
        server_->set_http_callback(std::bind(&LuaHttpServer::HandleRequest, this, std::placeholders::_1));
        set_event_callback(nref);
        return ERROR_OK;
    }

    void Stop() {
        if (server_) {
            server_->Stop();
        }
        set_event_callback(LUA_REFNIL);
    }
  private:
    void HandleRequest(const http::HttpMessage &request) {
        if (nref_ == LUA_REFNIL) return;

        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, nref_);
        if (!lua_isfunction(L, -1)) {
            log_warning("Http server can not found request handler funciton!");
            lua_pop(L, 1);
            return;
        }
        LuaState S{ L };
        S << request;
        luaL_pcall(L, 1, 0);
    }
  private:
    app::AppContainer * app_;
    std::unique_ptr<http::HttpServer> server_;
    int nref_;
};

static void http_response_callback(lua_State* L, int nref, const http::client::Response &response) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, nref);
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    if (!lua_isfunction(L, -1)) {
        log_warning("Http response can not found callback funciton!");
        lua_pop(L, 1);
        return;
    }
    LuaState S{ L };
    S << response;
    luaL_pcall(L, 1, 0);
}


static int http_request(lua_State *L) {
    auto app = lua_getapp(L);
    lua_State *LL = app->get<lua_State>();
    luaL_argcheck(L, lua_type(L, 1) == LUA_TTABLE, 1, "table expected");
    luaL_argcheck(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "function expected");
    lua_pushvalue(L, 1);
    http::client::Request req;
    LuaState S = { L };
    S >> req;
    lua_pop(L, 1);
    lua_pushvalue(L, 2);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    auto callback = std::bind(http_response_callback, LL, nref, std::placeholders::_1);
    auto request_guid = app->get<http::HttpClient>()->Request(req, callback);
    if (request_guid == 0) {
        luaL_unref(L, LUA_REGISTRYINDEX, nref);
        lua_pushnil(L);
        lua_pushstring(L, tinynet_strerror(ERROR_HTTP_REQUEST));
        return 2;
    }
    lua_pushidentifier(L, request_guid);
    return 1;
}

static LuaHttpServer* luaL_checkserver(lua_State *L, int idx) {
    return (LuaHttpServer*)luaL_checkudata(L, idx, HTTP_SERVER_META_TABLE);
}

static int http_server_new(lua_State *L) {
    auto app = lua_getapp(L);
    auto ptr = lua_newuserdata(L, sizeof(LuaHttpServer));
    new(ptr) LuaHttpServer(app);
    luaL_getmetatable(L, HTTP_SERVER_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int http_server_delete(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    server->~LuaHttpServer();
    return 0;
}

static int http_server_start(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    net::ServerOptions opts;
    int type = lua_type(L, 2);
    switch (type) {
    case LUA_TNUMBER: {
        opts.listen_port = luaL_checkint(L, 2);
        opts.listen_ip = "0.0.0.0";
        break;
    }
    case LUA_TSTRING: {
        size_t len;
        const char *addr = luaL_checklstring(L, 2, &len);
        opts.listen_url.assign(addr, len);
        break;
    }
    case LUA_TTABLE: {
        lua_pushvalue(L, 2);
        LuaState S{ L };
        S >> opts;
        lua_pop(L, 1);
        break;
    }
    default:
        return luaL_argerror(L, 2, "[port] number or [url] string or [options] table expected");
    }
    luaL_argcheck(L, lua_type(L, 3) == LUA_TFUNCTION, 3, "function expected");
    lua_pushvalue(L, 3);
    int nref = luaL_ref(L, LUA_REGISTRYINDEX);
    int err = server->Start(opts, nref);
    if (err == 0) {
        return 0;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, nref);
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static int http_server_stop(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    server->Stop();
    return 0;
}

static int http_server_send_response(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    luaL_argcheck(L, lua_type(L, 2) == LUA_TTABLE, 2, "table expected");
    http::ZeroCopyHttpMessage response;
    response.type = http::HTTP_RESPONSE;
    LuaState S{ L };
    S >> response;
    bool res = server->get_server() && server->get_server()->SendResponse(response);
    lua_pushboolean(L, res);
    return 1;
}

static const luaL_Reg http_client_methods[] = {
    { "request", http_request },
    { 0, 0 }
};

static const luaL_Reg http_server_meta_methods[] = {
    {"start", http_server_start},
    {"stop", http_server_stop},
    {"send_response", http_server_send_response },
    {"__gc", http_server_delete },
    { 0, 0 }
};

static const luaL_Reg http_server_methods[] = {
    {"new", http_server_new},
    { 0, 0 }
};

LUALIB_API int luaopen_http(lua_State *L) {
    luaL_newmetatable(L, HTTP_SERVER_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, http_server_meta_methods, 0);
    lua_pop(L, 1);

    lua_newtable(L);
    // http.client
    lua_pushstring(L, "client");
    lua_newtable(L);
    luaL_setfuncs(L, http_client_methods, 0);
    lua_rawset(L, -3);
    // http.server
    lua_pushstring(L, "server");
    lua_newtable(L);
    luaL_setfuncs(L, http_server_methods, 0);
    lua_rawset(L, -3);
    return 1;
}
