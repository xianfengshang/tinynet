// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef __linux__
#include <unistd.h>
#include "poller_epoll.h"
#include "event_loop.h"
#include "poller.h"
#include "base/net_types.h"
#include "util/net_utils.h"

namespace tinynet {
namespace net {
PollerEpoll::PollerEpoll(EventLoop* loop)
    :PollerImpl(loop),
     efd_(-1) {
}

bool PollerEpoll::Init() {
    efd_ = epoll_create(MAX_EPOLL_EVENTS);
    if (efd_ == -1) {
        return false;
    }
    return true;
}

void PollerEpoll::Stop() {
    if (efd_ != -1) {
        close(efd_);
        efd_ = -1;
    }
}

const char* PollerEpoll::name() {
    return "epoll";
}

int PollerEpoll::Poll(poll_event* events, int maxevents, int timeout_ms) {
    maxevents = (std::min)((int)MAX_EPOLL_EVENTS, maxevents);
    int res = epoll_wait(efd_, &events_[0], maxevents, timeout_ms);
    if (res <= 0) {
        return 0;
    }
    int nevents = res;
    for (int i = 0; i < nevents; ++i) {
        int mask = 0;
        struct epoll_event& event = events_[i];
        if (event.events & EPOLLIN) mask |= EVENT_READABLE;
        if (event.events & EPOLLOUT) mask |= EVENT_WRITABLE;
        if (event.events & EPOLLPRI) mask |= EVENT_READABLE;
        if (event.events & EPOLLRDHUP) mask |= EVENT_READABLE;
        if (event.events & EPOLLHUP) mask |= EVENT_READABLE | EVENT_WRITABLE | EVENT_ERROR;
        if (event.events & EPOLLERR) mask |= EVENT_READABLE | EVENT_WRITABLE | EVENT_ERROR;
        events[i].fd = event.data.fd;
        events[i].mask = mask;
    }
    return nevents;
}

int PollerEpoll::Add(int fd, int mask) {
    struct epoll_event event = { 0 };
    event.events = EPOLLET;
    event.data.fd = fd;
    int old_mask = event_loop_->get_poller()->GetEvents(fd);
    int op = old_mask == EVENT_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    mask |= old_mask;
    if (mask & EVENT_READABLE) event.events |= (EPOLLIN | EPOLLRDHUP);
    if (mask & EVENT_WRITABLE) event.events |= EPOLLOUT;
    if (old_mask == EVENT_NONE) {
        if (NetUtils::SetNonBlocking(fd) != 0) return -1;
    }
    int err = epoll_ctl(efd_, op, fd, &event);
    return err;
}

int PollerEpoll::Del(int fd, int mask) {
    struct epoll_event event = { 0 };
    event.events = EPOLLET;
    event.data.fd = fd;
    int old_mask = event_loop_->get_poller()->GetEvents(fd);
    int new_mask = old_mask & (~mask);
    if (new_mask & EVENT_READABLE) event.events |= (EPOLLIN | EPOLLRDHUP);
    if (new_mask & EVENT_WRITABLE) event.events |= EPOLLOUT;
    int op = new_mask == EVENT_NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    int err = epoll_ctl(efd_, op, fd, &event);
    return err;
}
}
}
#endif
