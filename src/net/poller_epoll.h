// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#ifdef __linux__
#include <string>
#include <array>
#include <sys/epoll.h>
#include "poller_impl.h"

namespace tinynet {
class EventLoop;
namespace net {

class PollerEpoll : public PollerImpl {
  public:
    PollerEpoll(EventLoop* loop);
  public:
    virtual bool Init() override;
    virtual void Stop() override;
    virtual const char* name() override;
    virtual int Add(int fd, int mask) override;
    virtual int Del(int fd, int mask)  override;
  public:
    int Poll(poll_event* events, int maxevents, int timeout_ms) override;
  private:
    const static size_t MAX_EPOLL_EVENTS = 1024;
  private:
    std::array<struct epoll_event, MAX_EPOLL_EVENTS> events_;
    int efd_;
};
}
}
#endif
