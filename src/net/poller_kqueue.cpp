// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef __FreeBSD__
#include "poller_kqueue.h"
#include "event_loop.h"
#include "poller.h"
#include "base/net_types.h"
#include "util/net_utils.h"
namespace tinynet {
namespace net {
PollerKqueue::PollerKqueue(EventLoop* loop)
    :PollerImpl(loop),
     kfd_(-1) {
    static_assert(false, "Warning: this code has not been tested, please remove this line and compile again.");
}

bool PollerKqueue::Init() {
    kfd_ = kqueue();
    if (kfd_ == -1) {
        return false;
    }
    return true;
}

void PollerKqueue::Stop() {
    if (kfd_ != -1) {
        close(kfd_);
        kfd_ = -1;
    }
}

const char* PollerKqueue::name() {
    return "kqueue";
}

int PollerKqueue::Poll(poll_event* events, int maxevents, int timeout_ms) {
    struct timespec timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_nsec = (timeout_ms % 1000) * 1000 * 1000;

    maxevents = (std::min)((int)MAX_KQUEUE_EVENTS, maxevents);
    int res = kevent(kfd_, NULL, 0, &events_[0], maxevents, &timeout);
    if (res <= 0) {
        return 0;
    }
    int nevents = res;
    for (int i = 0; i < nevents; ++i) {
        int mask = 0;
        struct kevent& event = events_[i];
        if (event->filter == EVFILT_READ) mask |= EVENT_READABLE;
        if (event->filter == EVFILT_WRITE) mask |= EVENT_WRITABLE;
        events[i].fd = event.ident;
        events[i].mask = mask;
    }
    return nevents;
}

int PollerKqueue::Add(int fd, int mask) {
    int old_mask = event_loop_->get_poller()->GetEvents(fd);
    if (old_mask == EVENT_NONE) {
        if (NetUtils::SetNonBlocking(fd) != 0) return -1;
    }
    struct kevent event;
    if (mask & EVENT_READABLE) {
        EV_SET(&event, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
        if (kevent(kfd_, &event, 1, NULL, 0, NULL) == -1) return -1;
    }
    if (mask & EVENT_WRITABLE) {
        EV_SET(&event, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, NULL);
        if (kevent(kfd_, &event, 1, NULL, 0, NULL) == -1) return -1;
    }
    return 0;
}

int PollerKqueue::Del(int fd, int mask) {
    struct kevent event;
    if (mask & EVENT_READABLE) {
        EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(kfd_, &event, 1, NULL, 0, NULL);
    }
    if (mask & EVENT_WRITABLE) {
        EV_SET(&event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        kevent(kfd_, &event, 1, NULL, 0, NULL);
    }
    return 0;
}
}
#endif
