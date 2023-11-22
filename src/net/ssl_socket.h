// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "event_loop.h"
#include "socket.h"
#include <memory>

namespace tinynet {
namespace net {
class SSLSocket;
typedef std::shared_ptr<SSLSocket> SSLSocketPtr;

class SSLSocket final
    : public Socket {
    friend class  SSListener;
  public:
    enum HandshakeStatus {
        HS_NONE,
        HS_HANDHAKING,
        HS_HANDSHAKED
    };
  public:
    SSLSocket(tinynet::EventLoop *loop, tinynet::SSLContext *ctx);
    SSLSocket(tinynet::EventLoop *loop, tinynet::SSLContext *ctx, int fd, int af, const std::string* peer_address);
    ~SSLSocket();
  public:
    void Close() override;
  private:
    void Open() override;
    void Readable() override;
    void Writable() override;
  public:
    //start ssl handshaking
    void Handshake();
  private:
    void SSLOpen();
    void SSLClose();
    void SSLReadable();
    void SSLWritable();
    void SSLHandshake();
  public:
    /**
     * @brief Set the ssl tunnel established callback
     *
     * @param cb
     */
    void set_estab_callback(EventCallback cb) { estab_callback_ = std::move(cb); }
  protected:
    SSLContext*   ssl_ctx_;
    int           handshake_status_;
    SSL*          ssl_;
    EventCallback estab_callback_;

};

}
}
