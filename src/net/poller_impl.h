// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <stdint.h>
#include "base/string_view.h"
#include "poller.h"
namespace tinynet {
class EventLoop;
namespace net {
class PollerImpl {
  public:
    PollerImpl(tinynet::EventLoop* loop) : event_loop_(loop) {};
    virtual ~PollerImpl() = default;
  public:
    virtual bool Init() = 0;
    virtual void Stop() = 0;
    virtual const char* name() = 0;
    virtual int Poll(poll_event* events, int maxevents, int timeout_ms) = 0;
    virtual int Add(int fd, int mask) = 0;
    virtual int Del(int fd, int mask) = 0;
  protected:
    tinynet::EventLoop* event_loop_{ nullptr };
};
}
}
