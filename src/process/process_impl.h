// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "process_types.h"
namespace tinynet {
class EventLoop;
namespace process {
class ProcessEventHandler;
class ProcessImpl {
  public:
    ProcessImpl(EventLoop* loop, ProcessEventHandler* handler) :
        event_loop_(loop),
        event_handler_(handler) {
    }
    virtual ~ProcessImpl() = default;
  public:
    virtual int Spawn(const ProcessOptions& options) = 0;
    virtual void Close() = 0;
    virtual int Kill(int signum) = 0;
    virtual int GetPid() = 0;
  protected:
    EventLoop* event_loop_;
    ProcessEventHandler* event_handler_;
};
}
}
