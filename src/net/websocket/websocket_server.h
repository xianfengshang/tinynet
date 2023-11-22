// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <functional>
#include <memory>
#include "net/event_loop.h"
#include "net/socket_server.h"
#include "websocket_protocol.h"
#include "net/ssl_context.h"

namespace tinynet {
namespace websocket {
namespace server {
enum WebSocketSessionEventType {
    WEBSOCKET_SESSION_EVENT_NONE,
    WEBSOCKET_SESSION_EVENT_ON_OPEN,
    WEBSOCKET_SESSION_EVENT_ON_MESSAGE,
    WEBSOCKET_SESSION_EVENT_ON_ERROR,
    WEBSOCKET_SESSION_EVENT_ON_CLOSE
};

struct WebSocketSessionEvent {
    WebSocketSessionEventType type{ WEBSOCKET_SESSION_EVENT_NONE };
    int64_t guid{ 0 };
    const WebSocketMessage* msg{ nullptr };
    int err{ 0 };
};
typedef std::function<void(const WebSocketSessionEvent& evt)> WebSocketSessionCallback;

}

class WebSocketServer :
    public net::SocketServer {
  public:
    WebSocketServer(EventLoop *loop);
    WebSocketServer(EventLoop *loop, SSLContext* ctx);
    ~WebSocketServer();
  public:
    int Start(net::ServerOptions& opts) override;

    void Stop() override;

    bool Send(int64_t channel_guid, const WebSocketMessage& msg);

    void Broadcast(const WebSocketMessage& msg);
  private:
    void Update();
  private:
    void websocket_session_onopen(int64_t session_guid);
    void websocket_session_onclose(int64_t session_guid);
    void websocket_session_onmessage(int64_t session_guid, const WebSocketMessage* msg);
    void websocket_session_onerror(int64_t session_guid, int err);
  public:
    void set_websocket_session_callback(const server::WebSocketSessionCallback& cb) { websocket_session_callback_ = cb; }

    int get_keepalive_ms() const { return opts_.keepalive_ms; }

    void set_keepalive_ms(int value) { opts_.keepalive_ms = value; }

    int get_max_packet_size() const { return opts_.max_packet_size; }

    void set_max_packet_size(int value) { opts_.max_packet_size = value; }
  public:
    net::SocketChannelPtr CreateChannel(net::SocketPtr sock) override;
    void CloseSession(int64_t session_guid);
  private:
    server::WebSocketSessionCallback websocket_session_callback_{ nullptr };
    int64_t update_timer_{ 0 };
};
}
}
