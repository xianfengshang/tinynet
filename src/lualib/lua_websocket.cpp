// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_websocket.h"
#include "lua_helper.h"
#include "logging/logging.h"
#include "lua_compat.h"
#include "lua_proto_types.h"
#include "net/websocket/websocket_server.h"
#include "net/websocket/websocket.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"
#include "util/net_utils.h"
#include "base/error_code.h"
#include "net/websocket/websocket_protocol.h"
#include "lua_websocket_types.h"
#include "app/app_container.h"
#include "net/stream_socket.h"
#include "net/event_loop.h"
#include "lua_common_types.h"

#define  WS_SERVER_META_TABLE  "ws_server_meta_table"
#define  WS_CLIENT_META_TABLE "ws_client_meta_table"

using namespace tinynet;

class LuaWebSocket {
  public:
    LuaWebSocket(app::AppContainer * app, tinynet::websocket::WebSocketPtr web_socket, const net::ChannelOptions& opts) :
        app_(app),
        web_socket_(web_socket),
        opts_(opts),
        nref_(LUA_REFNIL) {
        if (!web_socket_) {
            web_socket_ = std::make_shared < tinynet::websocket::WebSocket >(app_->event_loop());
        }
        web_socket_->set_onopen_callback(std::bind(&LuaWebSocket::onopen, this));
        web_socket_->set_onclose_callback(std::bind(&LuaWebSocket::onclose, this));
        web_socket_->set_onerror_callback(std::bind(&LuaWebSocket::onerror, this, std::placeholders::_1));
        web_socket_->set_onmessage_callback(std::bind(&LuaWebSocket::onmessage, this, std::placeholders::_1));
    }
    ~LuaWebSocket() {
        set_event_callback(LUA_REFNIL);
    }
  public:
    tinynet::websocket::WebSocketPtr get_web_socket() { return web_socket_; }

    void set_event_callback(int nref) {
        if (nref_ != LUA_REFNIL && nref_ != nref) {
            lua_State *L = app_->get<lua_State>();
            luaL_unref(L, LUA_REGISTRYINDEX, nref_);
        }
        nref_ = nref;
    }
  public:
    bool Open() { return web_socket_->Open(opts_); }
    void Close() { return web_socket_->Close(0); }
    bool Send(const tinynet::websocket::WebSocketMessage& msg) { return web_socket_->Send(msg); }
  public:
    void onopen() {
        tinynet::lua::WebSocketEvent event;
        event.guid = web_socket_->get_guid();
        event.addr = web_socket_->get_peer_ip();
        event.type = "onopen";
        emit(event);
    }
    void onclose() {
        tinynet::lua::WebSocketEvent event;
        event.guid = web_socket_->get_guid();
        event.addr = web_socket_->get_peer_ip();
        event.type = "onclose";
        emit(event);
    }
    void onerror(int err) {
        tinynet::lua::WebSocketEvent event;
        event.guid = web_socket_->get_guid();
        event.addr = web_socket_->get_peer_ip();
        event.type = "onerror";
        event.data = tinynet_strerror(err);
        emit(event);
    }

    void onmessage(const tinynet::websocket::WebSocketMessage* msg) {
        tinynet::lua::WebSocketEvent event;
        event.guid = web_socket_->get_guid();
        event.addr = web_socket_->get_peer_ip();
        event.type = "onmessage";
        if (msg) {
            event.data = msg->data;
        }
        emit(event);
    }

    void emit(const tinynet::lua::WebSocketEvent& event) {
        if (nref_ == LUA_REFNIL) {
            return;
        }
        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, nref_);
        if (!lua_isfunction(L, -1)) {
            log_warning("Websocket emit event can not found callback function!");
            lua_pop(L, 1);
            return;
        }
        LuaState S = { L };
        S << event;
        luaL_pcall(L, 1, 0);
    }
  private:
    app::AppContainer* app_;
    tinynet::websocket::WebSocketPtr web_socket_;
    net::ChannelOptions opts_;
    int nref_;
};

class LuaWebSocketServer {
  public:
    LuaWebSocketServer(app::AppContainer* worker) :
        app_(worker),
        nref_(LUA_REFNIL) {
    }
    ~LuaWebSocketServer() {
        set_event_callback(LUA_REFNIL);
    }
  public:
    tinynet::websocket::WebSocketServer* get_server() { return server_.get(); }

    void set_event_callback(int nref) {
        if (nref_ != LUA_REFNIL && nref_ != nref) {
            lua_State *L = app_->get<lua_State>();
            luaL_unref(L, LUA_REGISTRYINDEX, nref_);
        }
        nref_ = nref;
    }
  public:
    void on_websocket_session_event(const tinynet::websocket::server::WebSocketSessionEvent& evt) {
        if (nref_ == LUA_REFNIL) return;

        tinynet::lua::WebSocketEvent event;
        event.guid = evt.guid;
        tinynet::websocket::WebSocketPtr session;
        if (server_ && (session = server_->GetChannel<tinynet::websocket::WebSocket>(evt.guid))) {
            event.addr = session->get_peer_ip();
        }
        switch (evt.type) {
        case websocket::server::WEBSOCKET_SESSION_EVENT_NONE: {
            break;
        }
        case websocket::server::WEBSOCKET_SESSION_EVENT_ON_OPEN: {
            event.type = "onopen";
            break;
        }
        case websocket::server::WEBSOCKET_SESSION_EVENT_ON_MESSAGE: {
            event.type = "onmessage";
            if (evt.msg) {
                event.data_ref = &evt.msg->data;
            }
            break;
        }
        case websocket::server::WEBSOCKET_SESSION_EVENT_ON_ERROR: {
            event.type = "onerror";
            event.data = tinynet_strerror(evt.err);
            break;
        }
        case websocket::server::WEBSOCKET_SESSION_EVENT_ON_CLOSE: {
            event.type = "onclose";
            break;
        }
        default:
            break;
        }
        emit(event);
    }
    void emit(const tinynet::lua::WebSocketEvent& event) {
        if (nref_ == LUA_REFNIL) {
            return;
        }
        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, nref_);
        if (!lua_isfunction(L, -1)) {
            log_warning("Websocket server emit event can not found callback function!");
            lua_pop(L, 1);
            return;
        }
        LuaState S = { L };
        S << event;
        luaL_pcall(L, 1, 0);
    }
  public:
    int Start(net::ServerOptions& opts, int nref) {
        int err;
        if (server_) {
            return ERROR_SERVER_STARTED;
        }
        server_.reset(new (std::nothrow) tinynet::websocket::WebSocketServer(app_->event_loop()));
        if (!server_) {
            return ERROR_OS_OOM;
        }
        if (!opts.listen_url.empty()) {
            if (!UriUtils::parse_address(opts.listen_url, &opts.listen_ip, &opts.listen_port)) {
                return ERROR_URI_UNRECOGNIZED;
            }
        }
        if ((err = server_->Start(opts))) {
            server_.reset();
            return err;
        }
        set_event_callback(nref);
        server_->set_websocket_session_callback(std::bind(&LuaWebSocketServer::on_websocket_session_event, this, std::placeholders::_1));
        return ERROR_OK;
    }

    void Stop() {
        if (server_) {
            server_->Stop();
        }
        set_event_callback(LUA_REFNIL);
    }

    bool Send(int64_t channel_guid, const websocket::WebSocketMessage& msg) {
        if (!server_) return false;
        return server_->Send(channel_guid, msg);
    }

    void Broadcast(const websocket::WebSocketMessage& msg) {
        if (!server_) return;
        server_->Broadcast(msg);
    }
  private:
    app::AppContainer * app_;
    std::unique_ptr<tinynet::websocket::WebSocketServer> server_;
    int nref_;
};


static LuaWebSocketServer* luaL_checkserver(lua_State *L, int idx) {
    return (LuaWebSocketServer*)luaL_checkudata(L, idx, WS_SERVER_META_TABLE);
}

static int ws_server_new(lua_State *L) {
    auto app = lua_getapp(L);
    auto ptr = lua_newuserdata(L, sizeof(LuaWebSocketServer));
    new(ptr) LuaWebSocketServer(app);
    luaL_getmetatable(L, WS_SERVER_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int ws_server_delete(lua_State *L) {
    LuaWebSocketServer *server = luaL_checkserver(L, 1);
    server->~LuaWebSocketServer();
    return 0;
}


static int ws_server_start(lua_State *L) {
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

static int ws_server_stop(lua_State* L) {
    auto server = luaL_checkserver(L, 1);
    server->Stop();
    return 0;
}

static int ws_server_send_bytes(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    int64_t session_guid = luaL_checkidentifier(L, 2);
    int type = lua_type(L, 3);
    websocket::WebSocketMessage msg;
    msg.opcode = websocket::Opcode::Binary;
    if (type == LUA_TSTRING) {
        msg.data_ref = luaL_checklstring(L, 3, &msg.data_len);
    } else if (type == LUA_TUSERDATA) {
        msg.bytes_ref = luaL_checkbytes(L, 3);
    } else {
        return luaL_argerror(L, 2, "string or bytes expected");
    }
    auto result = server->Send(session_guid, msg);
    lua_pushboolean(L, result);
    return 1;
}

static int ws_server_send_text(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    int64_t session_guid = luaL_checkidentifier(L, 2);
    websocket::WebSocketMessage msg;
    msg.opcode = websocket::Opcode::Text;
    msg.data_ref = luaL_checklstring(L, 3, &msg.data_len);
    auto result = server->Send(session_guid, msg);
    lua_pushboolean(L, result);
    return 1;
}

static int ws_server_broadcast_msg(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    websocket::WebSocketMessage msg;
    msg.opcode = websocket::Opcode::Binary;
    msg.data_ref = luaL_checklstring(L, 2, &msg.data_len);
    server->Broadcast(msg);
    return 0;
}

static int ws_server_broadcast_text(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    websocket::WebSocketMessage msg;
    msg.opcode = websocket::Opcode::Text;
    msg.data_ref = luaL_checklstring(L, 2, &msg.data_len);
    server->Broadcast(msg);
    return 0;
}

static int ws_server_get_session_size(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    lua_pushnumber(L, static_cast<lua_Number>(server->get_server()->channel_size()));
    return 1;
}

static int ws_server_get_client_ip(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    int64_t session_guid = luaL_checkidentifier(L, 2);
    auto session = server->get_server()->GetChannel<tinynet::websocket::WebSocket>(session_guid);
    std::string client_ip;
    if (session) {
        client_ip = session->get_peer_ip();
    }
    lua_pushstring(L, client_ip.c_str());
    return 1;
}

static int ws_server_close_session(lua_State *L) {
    auto server = luaL_checkserver(L, 1);
    int64_t session_guid = luaL_checkidentifier(L, 2);
    server->get_server()->CloseSession(session_guid);
    return 0;
}

static LuaWebSocket* luaL_checkwebsocket(lua_State *L, int idx) {
    return (LuaWebSocket*)luaL_checkudata(L, idx, WS_CLIENT_META_TABLE);
}

static int ws_client_new(lua_State *L) {
    auto app = lua_getapp(L);
    net::ChannelOptions opts;
    int type = lua_type(L, 1);
    switch (type) {
    case LUA_TNUMBER: {
        int port = luaL_checkint(L, 1);
        StringUtils::Format(opts.url, "ws://127.0.0.1:%d", port);
        break;
    }
    case LUA_TSTRING: {
        size_t len;
        const char *addr = luaL_checklstring(L, 1, &len);
        opts.url.assign(addr, len);
        break;
    }
    case LUA_TTABLE: {
        LuaState S{ L };
        S >> opts;
        break;
    }
    default:
        return luaL_argerror(L, 2, "[port] number or [url] string or [options] table expected");
    }

    auto ptr = lua_newuserdata(L, sizeof(LuaWebSocket));
    new(ptr) LuaWebSocket(app, nullptr, opts);
    luaL_getmetatable(L, WS_CLIENT_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int ws_client_delete(lua_State *L) {
    LuaWebSocket *client = luaL_checkwebsocket(L, 1);
    client->~LuaWebSocket();
    return 0;
}

static int ws_client_open(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    lua_pushboolean(L, web_socket->Open());
    return 1;
}

static int ws_client_close(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    web_socket->Close();
    return 0;
}

static int ws_client_on_event(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    int type = lua_type(L, 2);
    luaL_argcheck(L, type == LUA_TNIL || type == LUA_TFUNCTION, 2, "function expected!");
    int nref = LUA_REFNIL;
    if (type == LUA_TFUNCTION) {
        lua_pushvalue(L, 2);
        nref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    web_socket->set_event_callback(nref);
    return 0;
}

static int ws_client_send_bytes(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    int type = lua_type(L, 2);
    websocket::WebSocketMessage msg;
    msg.opcode = websocket::Opcode::Binary;
    if (type == LUA_TSTRING) {
        msg.data_ref = luaL_checklstring(L, 2, &msg.data_len);
    } else if (type == LUA_TUSERDATA) {
        msg.bytes_ref = luaL_checkbytes(L, 2);
    } else {
        return luaL_argerror(L, 2, "string or bytes expected");
    }
    auto result = web_socket->Send(msg);
    lua_pushboolean(L, result);
    return 1;
}

static int ws_client_send_text(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    websocket::WebSocketMessage msg;
    msg.opcode = websocket::Opcode::Text;
    msg.data_ref = luaL_checklstring(L, 2, &msg.data_len);
    auto result = web_socket->Send(msg);
    lua_pushboolean(L, result);
    return 1;
}

static int ws_client_get_peer_ip(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    auto &peer_ip = web_socket->get_web_socket()->get_peer_ip();
    lua_pushstring(L, peer_ip.c_str());
    return 1;
}

static int ws_client_is_closed(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    auto result = web_socket->get_web_socket()->get_socket()->is_closed();
    lua_pushboolean(L, result);
    return 1;
}

static int ws_client_is_connected(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    auto result = web_socket->get_web_socket()->get_socket()->is_connected();
    lua_pushboolean(L, result);
    return 1;
}

static int ws_client_is_connecting(lua_State *L) {
    auto web_socket = luaL_checkwebsocket(L, 1);
    auto result = web_socket->get_web_socket()->get_socket()->is_connecting();
    lua_pushboolean(L, result);
    return 1;
}

static const luaL_Reg ws_client_methods[] = {
    { "new", ws_client_new},
    { 0, 0 }
};

static const luaL_Reg ws_client_meta_methods[] = {
    { "open", ws_client_open},
    { "close", ws_client_close},
    { "on_event", ws_client_on_event},
    { "send_bytes", ws_client_send_bytes},
    { "send_text", ws_client_send_text},
    { "get_peer_ip", ws_client_get_peer_ip},
    { "is_closed", ws_client_is_closed},
    { "is_connected", ws_client_is_connected},
    { "is_connecting", ws_client_is_connecting},
    {"__gc", ws_client_delete },
    { 0, 0 }
};

static const luaL_Reg ws_server_meta_methods[] = {
    {"start", ws_server_start},
    {"stop", ws_server_stop},
    {"send_bytes", ws_server_send_bytes },
    {"send_text", ws_server_send_text },
    {"broadcast_msg", ws_server_broadcast_msg },
    {"broadcast_text", ws_server_broadcast_text },
    {"get_session_size", ws_server_get_session_size},
    {"get_client_ip", ws_server_get_client_ip},
    {"close_session", ws_server_close_session},
    {"__gc", ws_server_delete },
    { 0, 0 }
};

static const luaL_Reg ws_server_methods[] = {
    {"new", ws_server_new},
    { 0, 0 }
};

LUALIB_API int luaopen_ws(lua_State *L) {
    luaL_newmetatable(L, WS_SERVER_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, ws_server_meta_methods, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, WS_CLIENT_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, ws_client_meta_methods, 0);
    lua_pop(L, 1);

    lua_newtable(L);
    // ws.client
    lua_pushstring(L, "client");
    lua_newtable(L);
    luaL_setfuncs(L, ws_client_methods, 0);
    lua_rawset(L, -3);
    // ws.server
    lua_pushstring(L, "server");
    lua_newtable(L);
    luaL_setfuncs(L, ws_server_methods, 0);
    lua_rawset(L, -3);
    return 1;
}
