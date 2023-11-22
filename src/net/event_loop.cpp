// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "event_loop.h"
#include "base/unique_id.h"
#include "logging/logging.h"
#include "util/net_utils.h"
#include "util/process_utils.h"
namespace tinynet {

EventLoop::EventLoop():
    delete_id_alloc_(true),
    stop_(0),
    time_(STime_ms()),
    thread_id_(ProcessUtils::get_tid()) {
    wakeup_fds[0] = -1;
    wakeup_fds[1] = -1;
}

EventLoop::~EventLoop() {
    if (!delete_id_alloc_) id_alloc_.release();
}

int EventLoop::Init() {
    timer_.reset(new(std::nothrow) TimerManager(this));
    if (!timer_) return 1;
    timer_->Init();

    poller_.reset(new(std::nothrow) net::Poller(this));
    if (!poller_) return 1;
    poller_->Init();

    task_.reset(new (std::nothrow) TaskManager(this));
    if (!task_) return 1;
    task_->Init();

#ifdef _WIN32
    int ret = NetUtils::SocketPair(AF_INET, SOCK_STREAM, 0, wakeup_fds);
#else
    int ret = NetUtils::SocketPair(AF_UNIX, SOCK_STREAM, 0, wakeup_fds);
#endif
    if (ret == -1) return 1;

    poller_->Add(wakeup_fds[1], net::EVENT_READABLE, std::bind(&EventLoop::OnWakeup, this));
    return 0;
}

void EventLoop::SetIdAllocator(IdAllocator* id_allocator, bool auto_delete) {
    if (id_alloc_ != nullptr && !delete_id_alloc_) {
        id_alloc_.release();
    }
    id_alloc_.reset(id_allocator);
    delete_id_alloc_ = auto_delete;
}

int64_t EventLoop::NewUniqueId() {
    if (id_alloc_) return id_alloc_->NextId();
    return tinynet::NewUniqueId();
}

void EventLoop::Stop() {
    if (wakeup_fds[0] != -1)
        NetUtils::Close(wakeup_fds[0]);
    if (wakeup_fds[1] != -1) {
        poller_->Del(wakeup_fds[1], net::EVENT_READABLE);
        NetUtils::Close(wakeup_fds[1]);
    }

    timer_->Stop();
    poller_->Stop();
    task_->Stop();
}

TimerId EventLoop::AddTimer(uint64_t timeout, uint64_t repeat, TimerCallback callback) {
    return timer_->AddTimer(timeout, repeat, callback);
}

void EventLoop::ClearTimer(TimerId& timerId) {
    timer_->ClearTimer(timerId);
}

int EventLoop::Run(int mode /* = RUN_FOREVER */) noexcept {
    if (mode == RUN_FOREVER)
        thread_id_ = ProcessUtils::get_tid();
    while (!stop_) {
        time_ = STime_ms();
        int timeout_ms = mode == RUN_NOWAIT ? 0 : GetBackendTimeout();
        poller_->Poll(timeout_ms);
        time_ = STime_ms();
        timer_->Run();
        task_->Run();
        if (mode != RUN_FOREVER) break;
    }
    return 0;
}

int EventLoop::AddEvent(int fd, int mask, net::EventCallback callback) {
    return poller_->Add(fd, mask, callback);
}

void EventLoop::ClearEvent(int fd, int mask) {
    poller_->Del(fd, mask);
}

TaskId EventLoop::AddTask(TaskFunc task_func) {
    auto taskId = task_->AddTask(task_func);
    if (thread_id_ != ProcessUtils::get_tid())
        Wakeup();
    return taskId;
}

void EventLoop::CancelTask(TaskId& taskId) {
    task_->CancelTask(taskId);
}

TaskId EventLoop::AddSignal(int signum, TaskFunc sigfunc) {
    return task_->AddSignal(signum, sigfunc);
}

void EventLoop::ClearSignal(int signum, TaskId& sigtid) {
    return task_->ClearSignal(signum, sigtid);
}

void EventLoop::Signal(int signum) {
    task_->Signal(signum);

    if (thread_id_ != ProcessUtils::get_tid())
        Wakeup();
}

void EventLoop::Wakeup() {
    if (wakeup_fds[0] == -1) return;
    char buf[1] = { 0 };
    NetUtils::Write(wakeup_fds[0], buf, sizeof(buf));
}

int EventLoop::GetBackendTimeout() {
    if (!task_->Empty()) return 0;
    int timeout = timer_->NearestTimeout();
    if (timeout < 0) {
        return MAX_BACKEND_TIMEOUT;
    } else if (timeout < MAX_BACKEND_TIMEOUT) {
        return timeout;
    } else {
        return MAX_BACKEND_TIMEOUT;
    }
}

void EventLoop::OnWakeup() {
    char buf[16];
    for (;;) {
        int nread = NetUtils::Read(wakeup_fds[1], buf, sizeof(buf));
        if (nread < (int)sizeof(buf)) break;
    }
    poller_->Add(wakeup_fds[1], net::EVENT_READABLE, std::bind(&EventLoop::OnWakeup, this));
}
}
