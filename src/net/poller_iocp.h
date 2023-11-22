// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#ifdef _WIN32
#include <string>
#include <array>
#include "poller_impl.h"
#include <WinSock2.h>

namespace tinynet {
class EventLoop;
namespace net {

class PollerIocp : public PollerImpl {
  public:
    PollerIocp(EventLoop* loop);
  public:
    virtual bool Init() override;
    virtual void Stop() override;
    virtual const char* name() override;
    virtual int Add(int fd, int mask) override;
    virtual int Del(int fd, int mask)  override;
  public:
    int Poll(poll_event* events, int maxevents, int timeout_ms) override;
  private:
    const static size_t MAX_OVERLAMPPED_ENTRIES = 1024;
  private:
    HANDLE handle_;
    std::array<OVERLAPPED_ENTRY, MAX_OVERLAMPPED_ENTRIES> entries_;
};
}
}
#endif
