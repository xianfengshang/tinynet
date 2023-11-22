// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/socket_channel.h"
#include "websocket_protocol.h"

namespace tinynet {
namespace http {
class HttpCodec;
struct HttpMessage;
}
namespace websocket {

class WebSocketCodec;
class WebSocketServer;

typedef std::function<void()> onopen_callback;
typedef std::function<void(const WebSocketMessage* msg)> onmessage_callback;
typedef std::function<void(int err)> onerror_callback;
typedef std::function<void()> onclose_callback;

class WebSocket;
typedef std::shared_ptr<WebSocket> WebSocketPtr;

class WebSocket:
    public net::SocketChannel {
    friend WebSocketServer;
  public:
    WebSocket(net::SocketPtr sock, WebSocketServer* server);
    WebSocket(EventLoop* loop);
    ~WebSocket();
  private:
    void Dispose(bool disposed);
  public:
    bool Open(net::ChannelOptions& opts);
    bool Open(const std::string& url);
    bool Send(const WebSocketMessage& msg);
  public:
    void OnOpen() override;
    void OnRead() override;
    void OnError(int err) override;
    void OnClose() override;
  protected:
    void HandleHandshake(const tinynet::http::HttpMessage& msg);
    void OnHandshakeReq(const tinynet::http::HttpMessage& msg);
    void OnHandshakeAck(const tinynet::http::HttpMessage& msg);
    void HandleMessage(const WebSocketMessage* msg);
    std::string EncryptKey(const std::string& key);
  public:
    bool handshaked() const { return handshake_; }

    bool is_alive();

    net::SocketPtr get_socket() { return socket_; }

    void set_onopen_callback(onopen_callback cb) { onopen_callback_ = cb; }

    void set_onmessage_callback(onmessage_callback cb) { onmessage_callback_ = cb; }

    void set_onerror_callback(onerror_callback cb) { onerror_callback_ = cb; }

    void set_onclose_callback(onclose_callback cb) { onclose_callback_ = cb; }

    const std::string& get_peer_ip() const { return peer_ip_; }

  public:
    void Update();
    void Handshake();
    void Ping();
    void Pong(const std::string &data);
  private:
    void ParseAddress(const tinynet::http::HttpMessage& msg);
  protected:
    std::unique_ptr<http::HttpCodec>	codec_http_;
    std::unique_ptr<WebSocketCodec>		codec_websocket_;
    std::string			url_;
    std::string			sec_ws_key_;
    std::string			peer_ip_;
    bool				handshake_;
    int64_t				keepalive_time_;
    int64_t				update_timer_;
    int64_t				ping_time_;
    onopen_callback		onopen_callback_;
    onmessage_callback	onmessage_callback_;
    onerror_callback	onerror_callback_;
    onclose_callback	onclose_callback_;
};

}
}
