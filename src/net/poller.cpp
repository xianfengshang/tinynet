// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "poller.h"
#include "poller_impl.h"
#include "socket.h"
#include "event_loop.h"
#include "base/error_code.h"
#if defined(_WIN32)
#include "poller_iocp.h"
#elif defined(__linux__)
#include "poller_epoll.h"
#elif defined(__FreeBSD__)
#include "poller_kqueue.h"
#else
#include "poller_select.h"
#endif
namespace tinynet {
namespace net {
Poller::Poller(tinynet::EventLoop* loop):
    event_loop_(loop) {
    PollerImpl* pImpl = nullptr;
#if defined(_WIN32)
    pImpl = new(std::nothrow) PollerIocp(loop);
#elif defined(__linux__)
    pImpl = new(std::nothrow) PollerEpoll(loop);
#elif defined(__FreeBSD__)
    pImpl = new(std::nothrow) PollerKqueue(loop);
#else
    pImpl = new(std::nothrow) PollerSelect(loop);
#endif
    impl_.reset(pImpl);
}

Poller::~Poller() = default;

void Poller::Init() {
    impl_->Init();
}

void Poller::Stop() {
    impl_->Stop();
}

const char* Poller::name() {
    return impl_->name();
}

int Poller::Poll(int timeout_ms) {
    int nevents = impl_->Poll(&poll_events_[0], MAX_POLL_EVENT, timeout_ms);
    for (int i = 0; i < nevents; ++i) {
        int fd = poll_events_[i].fd;
        int mask = poll_events_[i].mask;
        auto it = events_.find(fd);
        if (it != events_.end()) {
            Invoke(it->second.callback, fd, mask);
        }
    }
    return 0;
}

int Poller::Add(int fd, int mask, EventCallback callback) {
    if (impl_->Add(fd, mask) != 0) {
        return -1;
    }
    auto& ev = events_[fd];
    ev.mask |= mask;
    ev.callback = std::move(callback);
    return 0;
}

int Poller::Del(int fd, int mask) {
    auto it = events_.find(fd);
    if (it == events_.end()) {
        return -1;
    }
    impl_->Del(fd, mask);

    auto& ev = it->second;

    ev.mask &= ~mask;

    if (ev.mask == EVENT_NONE) {
        events_.erase(it);
    }
    return 0;
}

int Poller::GetEvents(int fd) {
    auto it = events_.find(fd);
    if (it == events_.end()) {
        return EVENT_NONE;
    }
    return it->second.mask;
}

}
}
