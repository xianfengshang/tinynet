// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "event_loop.h"
#include "socket.h"
#include <memory>

namespace tinynet {
namespace net {
class StreamSocket;
typedef std::shared_ptr<StreamSocket> StreamSocketPtr;

class StreamSocket final
    : public Socket {
    friend class  StreamListener;
  public:
    StreamSocket(tinynet::EventLoop *loop);
    StreamSocket(tinynet::EventLoop *loop, int fd, int af, const std::string* peer_address);
    ~StreamSocket();
  private:
    void Open() override;
    void Readable() override;
    void Writable() override;
  private:
    void StreamOpen();
};

}
}