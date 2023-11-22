// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <set>
#include <array>

namespace tinynet {
class EventLoop;
typedef std::function<void()> TimerCallback;
typedef int64_t TimerId;

constexpr TimerId INVALID_TIMER_ID = 0;

class TimerManager {
  public:
    TimerManager(EventLoop *loop);
    ~TimerManager();
  public:
    void Init();
    void Stop();
    void Run();
    int NearestTimeout();
  public:
    TimerId AddTimer(uint64_t timeout, uint64_t repeat,
                     TimerCallback ontimeout,TimerCallback onstop = TimerCallback());
    void ClearTimer(TimerId& timerId);
  private:
    struct TimerEvent {
        TimerId timerid{ 0 };
        uint64_t timeout{ 0 };
        uint64_t interval{ 0 };
        TimerCallback ontimeout;
        TimerCallback onstop;
    };
    typedef std::shared_ptr<TimerEvent> TimerEventPtr;
    typedef std::pair<uint64_t, TimerId> TimerEntry;
  private:
    static const size_t MAX_TIMER_EVENT = 16;
    using TIMER_ENTRY_SET = std::set<TimerEntry>;
    using TIMER_ID_ARRAY = std::array<TimerId, MAX_TIMER_EVENT>;
    using TIMER_EVENT_MAP = std::unordered_map<TimerId, TimerEventPtr>;
  private:
    EventLoop * event_loop_;
    TIMER_EVENT_MAP timers_;
    TIMER_ENTRY_SET entries_;
    TIMER_ID_ARRAY events_;
};
}
