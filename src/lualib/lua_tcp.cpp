// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_tcp.h"
#include "net/stream_socket.h"
#include "net/stream_listener.h"
#include "app/app_container.h"
#include "lua_helper.h"
#include "lua_compat.h"
#include "lua_socket_types.h"
#include "lua_types.h"
#include "lua_proto_types.h"
#include "base/error_code.h"
#include "logging/logging.h"
#include "base/io_buffer_stream.h"
#include "util/uri_utils.h"
#include "util/net_utils.h"
#include "lua_common_types.h"

using namespace tinynet;
#define  TCP_SOCKET_META_TABLE "tcp_socket_meta_table"

typedef std::function<void(int err)> ConnectCallback;

class LuaSocket {
  public:
    LuaSocket(app::AppContainer* app):
        app_(app),
        nref_(LUA_REFNIL) {
        guid_ = app->event_loop()->NewUniqueId();
    }
    LuaSocket(app::AppContainer* app, tinynet::net::SocketPtr socket):
        app_(app),
        nref_(LUA_REFNIL) {
        set_socket(socket);
        guid_ = app->event_loop()->NewUniqueId();
    }
    ~LuaSocket() {
        set_event_callback(LUA_REFNIL);
    }
  public:
    LuaSocket(const LuaSocket &o) = delete;
    LuaSocket& operator = (const LuaSocket &o) = delete;
  public:
    void HandleAccept(std::shared_ptr<tinynet::net::Socket> sock) {
        if (nref_ == LUA_REFNIL) {
            return;
        }
        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, nref_);
        if (!lua_isfunction(L, -1)) {
            // no callback
            lua_pop(L, 1);
            return;
        }
        lua_newtable(L);
        lua_pushstring(L, "type");
        lua_pushstring(L, "onaccept");
        lua_rawset(L, -3);
        lua_pushstring(L, "data");
        auto ptr = lua_newuserdata(L, sizeof(LuaSocket));
        new(ptr) LuaSocket(app_, sock);
        luaL_getmetatable(L, TCP_SOCKET_META_TABLE);
        lua_setmetatable(L, -2);
        lua_rawset(L, -3);
        luaL_pcall(L, 1, 0);
    }
    void HandleConnect() {
        tinynet::lua::TcpSocketEvent event;
        event.type = "onopen";
        emit(event);
    }
    void HandleRead() {
        auto socket = get_socket();
        if (!socket) {
            return;
        }
        tinynet::lua::TcpSocketEvent event;
        event.type = "onread";
        event.data.resize(socket->rbuf()->size());
        socket->rbuf()->read(&event.data[0], event.data.size());
        emit(event);
    }
    void HandleWrite() {
        auto socket = get_socket();
        if (!socket) return;
        tinynet::lua::TcpSocketEvent event;
        event.type = "onwrite";
        emit(event);
    }

    void HandleError(int err) {
        tinynet::lua::TcpSocketEvent event;
        event.type = "onerror";
        event.data = tinynet_strerror(err);
        emit(event);
    }
  public:
    int Connect(std::string& host, int port) {
        if (socket_) return false;
        auto socket = app_->event_loop()->NewObject<tinynet::net::StreamSocket>();
        int err = socket->Connect(host, port);
        if (err == ERROR_OK) {
            set_socket(socket);
        }
        return err;
    }

    int Send(const void* buffer, size_t len) {
        auto socket = get_socket();
        if (!socket || !socket->is_connected()) {
            return ERROR_SOCKET_NOT_CONNECTED;
        }
        socket->Write(buffer, len);
        return ERROR_OK;
    }
    void Close() {
        if (socket_)socket_->Close();
    }

    int Listen(const lua::ServerOptions& opts) {
        if (socket_) return ERROR_SERVER_STARTED;
        std::string listen_ip;
        int listen_port;
        if (opts.listen_url.empty()) {
            listen_ip = opts.listen_ip;
            listen_port = opts.listen_port;
        } else {
            if (!UriUtils::parse_address(opts.listen_url, &listen_ip, &listen_port)) {
                return ERROR_URI_UNRECOGNIZED;
            }
        }
        int flags = 0;
        if (opts.reuseport) flags |= TCP_FLAGS_REUSEPORT;
        if (opts.ipv6only) flags |= TCP_FLAGS_IPV6ONLY;
        auto listener = app_->event_loop()->NewObject<tinynet::net::StreamListener>();
        if (!listener) return ERROR_OS_OOM;
        int err =  listener->BindAndListen(listen_ip, listen_port, tinynet::net::ksomaxconn, flags);
        if (err == ERROR_OK) {
            set_listener(listener);
        }
        return err;
    }
  public:
    void emit(const tinynet::lua::TcpSocketEvent& event) {
        if (nref_ == LUA_REFNIL) {
            return;
        }
        lua_State *L = app_->get<lua_State>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, nref_);
        if (!lua_isfunction(L, -1)) {
            log_warning("Tcp socket emit event can not found callback function!");
            lua_pop(L, 1);
            return;
        }
        LuaState S = { L };
        S << event;
        luaL_pcall(L, 1, 0);
    }
  public:
    net::SocketStatus get_status() {
        auto sock = get_socket();
        if (sock) {
            return sock->get_status();
        }
        return net::SocketStatus::SS_UNSPEC;
    }
    void set_event_callback(int nref) {
        if (nref_ != LUA_REFNIL && nref_ != nref) {
            lua_State *L = app_->get<lua_State>();
            luaL_unref(L, LUA_REGISTRYINDEX, nref_);
        }
        nref_ = nref;
    }
    int64_t get_guid() const {
        return guid_;
    }

    tinynet::net::StreamSocketPtr get_socket() {
        return std::static_pointer_cast<tinynet::net::StreamSocket>(socket_);
    }

    tinynet::net::StreamListenerPtr get_listener() {
        return std::static_pointer_cast<tinynet::net::StreamListener> (socket_);
    }
  private:
    void set_socket(tinynet::net::SocketPtr sock) {
        if (sock) {
            sock->set_read_callback(std::bind(&LuaSocket::HandleRead, this));
            sock->set_write_callback(std::bind(&LuaSocket::HandleWrite, this));
            sock->set_conn_callback(std::bind(&LuaSocket::HandleConnect, this));
            sock->set_error_callback(std::bind(&LuaSocket::HandleError, this, std::placeholders::_1));
        }
        socket_ = std::move(sock);
    }
    void set_listener(tinynet::net::ListenerPtr  listener) {
        if (listener) {
            listener->set_conn_callback(std::bind(&LuaSocket::HandleAccept, this, std::placeholders::_1));
            listener->set_error_callback(std::bind(&LuaSocket::HandleError, this, std::placeholders::_1));
        }
        socket_ = std::move(listener);
    }
  protected:
    int64_t			guid_;
    app::AppContainer* app_;
    std::shared_ptr<tinynet::net::Socket> socket_;
    int nref_;
};

static LuaSocket* luaL_checktcp(lua_State* L, int idx) {
    return (LuaSocket*)luaL_checkudata(L, idx, TCP_SOCKET_META_TABLE);
}

static int tcp_socket_new(lua_State* L) {
    auto app = lua_getapp(L);
    auto ptr = lua_newuserdata(L, sizeof(LuaSocket));
    auto socket = new(ptr) LuaSocket(app);
    (void)socket;
    luaL_getmetatable(L, TCP_SOCKET_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int tcp_socket_delete(lua_State* L) {
    LuaSocket* socket =  luaL_checktcp(L, 1);
    socket->~LuaSocket();
    return 0;
}

static int tcp_socket_on_event(lua_State *L) {
    auto socket = luaL_checktcp(L, 1);
    int type = lua_type(L, 2);
    luaL_argcheck(L, type == LUA_TNIL || type == LUA_TFUNCTION, 2, "function expected!");

    int nref = LUA_REFNIL;
    if (type == LUA_TFUNCTION) {
        lua_pushvalue(L, 2);
        nref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    socket->set_event_callback(nref);
    return 0;
}


static int tcp_socket_close(lua_State* L) {
    auto socket = luaL_checktcp(L, 1);
    socket->Close();
    return 0;
}

static const char* SOCKET_STATUS_STRINGS[] = {
    "unspec",
    "connecting",
    "connected",
    "closed"
};
static_assert(sizeof(SOCKET_STATUS_STRINGS) / sizeof(SOCKET_STATUS_STRINGS[0]) == (size_t)net::SocketStatus::SS_MAX, "SOCKET_STATUS_STRINGS array length error");

static int tcp_socket_get_status(lua_State* L) {
    auto sock = luaL_checktcp(L, 1);
    lua_pushstring(L, SOCKET_STATUS_STRINGS[(int)sock->get_status()]);
    return 1;
}

static int tcp_socket_send(lua_State* L) {
    auto socket = luaL_checktcp(L, 1);
    size_t dataLen{ 0 };
    const char* data = luaL_checklstring(L, 2, &dataLen);
    int err = socket->Send(data, dataLen);
    if (err) {
        lua_pushstring(L, tinynet_strerror(err));
        return 1;
    }
    return 0;
}

static int tcp_socket_connect(lua_State* L) {
    auto socket = luaL_checktcp(L, 1);
    const char* host_str = luaL_checkstring(L, 2);
    int port = (int)luaL_checknumber(L, 3);
    std::string host(host_str);
    int err = socket->Connect(host, port);
    if (err) {
        lua_pushstring(L, tinynet_strerror(err));
        return 1;
    }
    return 0;
}

static int tcp_socket_listen(lua_State* L) {
    auto socket = luaL_checktcp(L, 1);
    lua::ServerOptions opts;
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
        LuaState S{ L };
        S >> opts;
        break;
    }
    default:
        return luaL_argerror(L, 2, "[port] number or [url] string or [options] table expected");
    }
    int err = socket->Listen(opts);
    if (err == ERROR_OK) {
        return 0;
    }
    lua_pushstring(L, tinynet_strerror(err));
    return 1;
}

static luaL_Reg meta_methods[] = {
    {"__gc", tcp_socket_delete},
    {"on_event", tcp_socket_on_event},
    {"connect", tcp_socket_connect},
    {"send", tcp_socket_send},
    {"close", tcp_socket_close},
    {"get_status", tcp_socket_get_status},
    {"listen", tcp_socket_listen},
    {0, 0}
};

static luaL_Reg methods[] = {
    {"new", tcp_socket_new},
    {0, 0}
};

LUALIB_API int luaopen_tcp(lua_State *L) {
    luaL_newmetatable(L, TCP_SOCKET_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta_methods, 0);
    lua_pop(L, 1);
    luaL_newlib(L, methods);
    return 1;
}
