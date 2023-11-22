// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>
#include "base/string_view.h"

namespace tinynet {
class EventLoop;
namespace net {
class PollerImpl;

typedef std::function<void(int fd, int mask)> EventCallback;

struct poll_event {
    int fd{ -1 };
    int mask{ 0 };
};

class Poller {
  public:
    Poller(tinynet::EventLoop* loop);
    ~Poller();
  public:
    void Init();

    void Stop();

    const char* name();

    int Poll(int timeout_ms);

    int Add(int fd, int mask, EventCallback callback);

    int Del(int fd, int mask);

    int GetEvents(int fd);
  private:
    struct fd_event {
        int mask{ 0 };
        EventCallback callback;
    };
  public:
    static const int MAX_POLL_EVENT = 16;
    using POLL_EVENT_ARRAY = std::array<poll_event, MAX_POLL_EVENT>;
    using FD_EVENT_MAP = std::unordered_map<int, fd_event>;
  private:
    FD_EVENT_MAP events_;
    POLL_EVENT_ARRAY  poll_events_;
    std::unique_ptr<PollerImpl> impl_;
    EventLoop* event_loop_;
};
}
}
