// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include "base/id_allocator.h"
#include "base/net_types.h"
#include "timer_manager.h"
#include "poller.h"
#include "task_manager.h"
#include "ssl_context.h"
namespace tinynet {
class EventLoop {
  public:
    EventLoop();
    ~EventLoop();
  public:
    enum RunMode {
        RUN_FOREVER,
        RUN_ONCE,
        RUN_NOWAIT
    };
    static const int MAX_BACKEND_TIMEOUT = 100;

    int Init();
    void Stop();
    int Run(int mode = RUN_FOREVER) noexcept;
    void Exit() { stop_ = 1; }
  public:
    void SetIdAllocator(IdAllocator* id_allocator, bool auto_delete = true);

    int64_t NewUniqueId();

    template<typename T, typename... Args>
    std::shared_ptr<T>
    NewObject(Args&&... args) { return std::make_shared<T>(this, std::forward<Args>(args)...); }

    int64_t Time() { return time_; }

    TimerId AddTimer(uint64_t timeout, uint64_t repeat, TimerCallback callback);

    void ClearTimer(TimerId& timerId);

    int AddEvent(int fd, int events, net::EventCallback callback);

    void ClearEvent(int fd, int mask);

    TaskId AddTask(TaskFunc task_func);

    void CancelTask(TaskId& taskId);

    TaskId AddSignal(int signum, TaskFunc sigfunc);

    void ClearSignal(int signum, TaskId& sigtid);

    void Signal(int signum);

    void Wakeup();
  public:
    TimerManager* get_timer() { return timer_.get(); }
    net::Poller* get_poller() { return poller_.get(); }
    TaskManager* get_task() { return task_.get(); }
    int thread_id() { return thread_id_; }
  private:
    int GetBackendTimeout();
    void OnWakeup();
  private:
    std::unique_ptr<TimerManager> timer_;
    std::unique_ptr<net::Poller>  poller_;
    std::unique_ptr<TaskManager>  task_;
    std::unique_ptr<IdAllocator>  id_alloc_;
    bool			delete_id_alloc_;
    std::atomic_int	stop_;
    int64_t         time_;
    int				wakeup_fds[2];
    int				thread_id_;
};
}
