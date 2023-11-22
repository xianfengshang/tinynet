#pragma once
#include "net/socket_channel.h"
#include "net/socket_server.h"
#include "http_protocol.h"
namespace tinynet {
namespace http {
class HttpCodec;

class HttpChannel :
    public net::SocketChannel {
  public:
    HttpChannel(net::SocketPtr sock, net::SocketServer *server);
    HttpChannel(EventLoop* loop);
    ~HttpChannel();
  public:
    void OnRead() override;
  public:
    bool SendResponse(const HttpMessage& resp);
    bool SendResponse(const ZeroCopyHttpMessage& resp);
  private:
    void ParseAddress(const tinynet::http::HttpMessage& msg);
  private:
    std::unique_ptr<HttpCodec> codec_;
    std::string peer_ip_;
};
}
}