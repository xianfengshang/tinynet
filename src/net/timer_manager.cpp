// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "timer_manager.h"
#include "logging/logging.h"
#include "base/unique_id.h"
#include "base/base.h"
#include "event_loop.h"
#include <algorithm>
#include "util/string_utils.h"
#include <string>

namespace tinynet {

TimerManager::TimerManager(EventLoop *loop) :
    event_loop_(loop),
    events_() {
}

TimerManager::~TimerManager() = default;

void TimerManager::Init() {
}

void TimerManager::Stop() {
    TIMER_ENTRY_SET entries(entries_);
    TimerId timerId;
    for (auto& entry: entries) {
        timerId = entry.second;
        ClearTimer(timerId);
    }
}


TimerId TimerManager::AddTimer(uint64_t timeout, uint64_t repeat, TimerCallback ontimeout,TimerCallback onstop /* = TimerCallback() */) {
    auto timeout_ms = event_loop_->Time() + timeout;
    auto timer_id = event_loop_->NewUniqueId();
    TimerEventPtr pEvent = std::make_shared<TimerEvent>();
    if (!pEvent) return  INVALID_TIMER_ID;

    pEvent->timerid = timer_id;
    pEvent->interval = repeat;
    pEvent->ontimeout = std::move(ontimeout);
    pEvent->onstop = std::move(onstop);
    pEvent->timeout = timeout_ms;

    bool result = timers_.emplace(timer_id, std::move(pEvent)).second;
    if (!result) return INVALID_TIMER_ID;
    entries_.insert(std::make_pair(timeout_ms, timer_id));
    return timer_id;
}

void TimerManager::ClearTimer(TimerId& timerId) {
    if (timerId == INVALID_TIMER_ID) return;
    auto it = timers_.find(timerId);
    if (it != timers_.end()) {
        TimerEventPtr timer(it->second);
        entries_.erase(std::make_pair(timer->timeout, timerId));
        timers_.erase(it);

        Invoke(timer->onstop);
    }
    timerId = INVALID_TIMER_ID;
}


void TimerManager::Run() {
    if (entries_.empty()) return;

    uint64_t now = (uint64_t)event_loop_->Time();
    if (now < entries_.begin()->first) return;

    TIMER_ENTRY_SET::iterator first = entries_.begin();
    TIMER_ENTRY_SET::iterator last = entries_.upper_bound(std::make_pair(now, (std::numeric_limits<uint64_t>::max)()));
    TIMER_ENTRY_SET::iterator it;
    size_t nevents = 0;
    for (it = first; it != last && nevents < MAX_TIMER_EVENT; ++it, ++nevents) {
        events_[nevents] = it->second;
    }
    entries_.erase(first, it);
    for (size_t i = 0; i < nevents; ++i) {
        auto timer_it = timers_.find(events_[i]);
        if (timer_it == timers_.end()) continue;
        std::weak_ptr<TimerEvent> week_timer(timer_it->second);
        Invoke(timer_it->second->ontimeout);
        if (week_timer.expired()) continue;
        auto pTimer = week_timer.lock();
        if (pTimer->interval == 0) {
            timers_.erase(pTimer->timerid);
            Invoke(pTimer->onstop);
        } else {
            pTimer->timeout += pTimer->interval;
            entries_.insert(std::make_pair(pTimer->timeout, pTimer->timerid));
        }
    }
}

int TimerManager::NearestTimeout() {
    if (entries_.empty()) return EventLoop::MAX_BACKEND_TIMEOUT;
    auto retval = (int64_t)entries_.begin()->first - event_loop_->Time();
    if (retval <= 0) {
        return 0;
    }
    return (int)retval;
}
}
