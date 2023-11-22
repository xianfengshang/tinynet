// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/event_loop.h"
#include "net/socket_server.h"
#include "http_protocol.h"
#include <functional>
#include <memory>

namespace tinynet {
namespace http {
namespace server {

typedef std::function<void(const HttpMessage& req)> HttpCallback;

}

class HttpServer :
    public net::SocketServer {
  public:
    HttpServer(EventLoop *loop);
    ~HttpServer();
  public:
    int Start(net::ServerOptions& opts) override;
    bool SendResponse(const HttpMessage& resp);
    bool SendResponse(const ZeroCopyHttpMessage& resp);
    void HandleRequest(int64_t session_guid, const HttpMessage& request);
  public:
    void set_http_callback(server::HttpCallback cb) { http_callback_ = std::move(cb); }
  public:
    net::SocketChannelPtr CreateChannel(net::SocketPtr sock) override;
  private:
    server::HttpCallback http_callback_;
};
}
}
