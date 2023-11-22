// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "poller_select.h"
#include "base/net_types.h"
#include "net/event_loop.h"
#include "base/runtime_logger.h"
#include "base/clock.h"
#include "util/net_utils.h"
#ifdef _WIN32
#include "base/winsock_manager.h"
#endif

namespace tinynet {
namespace net {
PollerSelect::PollerSelect(EventLoop* loop): PollerImpl(loop) {
}

bool PollerSelect::Init() {
    return true;
}

void PollerSelect::Stop() {
}

const char* PollerSelect::name() {
    return "select";
}

int PollerSelect::Poll(poll_event* events, int maxevents, int timeout_ms) {
    if (rfd_set_.empty() && wfd_set_.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        return 0;
    }

    int maxfd;
    FD_ZERO2(&rfd_);
    FD_ZERO2(&wfd_);
    maxfd = -1;
    for (auto& fd : rfd_set_) {
        FD_SET2(fd, &rfd_);
        if (fd > maxfd) maxfd = fd;
    }
    for (auto& fd : wfd_set_) {
        FD_SET2(fd, &wfd_);
        if (fd > maxfd) maxfd = fd;
    }
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    int res = select(maxfd + 1, &rfd_, &wfd_, NULL, &timeout);
    if (res <= 0) {
        return 0;
    }
    int nevents = 0;
    for (int i = 0; i <= maxfd; ++i) {
        int mask = 0;
        if (FD_ISSET(i, &rfd_)) {
            mask |= EVENT_READABLE;
        }
        if (FD_ISSET(i, &wfd_)) {
            mask |= EVENT_WRITABLE;
        }
        if (mask) {
            events[nevents].fd = i;
            events[nevents].mask = mask;
            ++nevents;
        }
    }
    return nevents;
}

int PollerSelect::Add(int fd, int mask) {
    if (fd >= FD_SETSIZE_EXTEND) return -1;
    int old_mask = event_loop_->get_poller()->GetEvents(fd);
    if (old_mask == EVENT_NONE) {
        if (NetUtils::SetNonBlocking(fd) != 0) return -1;
    }
    if (mask & EVENT_READABLE) rfd_set_.insert(fd);
    if (mask & EVENT_WRITABLE) wfd_set_.insert(fd);
    return 0;
}

int PollerSelect::Del(int fd, int mask) {
    if (mask & EVENT_READABLE) {
        rfd_set_.erase(fd);
    }
    if (mask & EVENT_WRITABLE) {
        wfd_set_.erase(fd);
    }
    return 0;
}

}
}
