// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <stdint.h>
#include <functional>
#include <deque>
#include <array>
#include <mutex>
#include <utility>

namespace tinynet {
typedef int64_t TaskId;
typedef std::function<void()> TaskFunc;

constexpr TaskId INVALID_TASK_ID = 0;

class EventLoop;
class TaskManager {
  public:
    TaskManager(EventLoop* loop);
    ~TaskManager();
  public:
    void Init();
    void Stop();
    void Run();
  public:
    TaskId AddTask(TaskFunc taskfunc);
    void CancelTask(TaskId& taskid);

    TaskId AddSignal(int signum, TaskFunc sigfunc);
    void ClearSignal(int signum, TaskId& sigtid);
    void Signal(int signum);
    bool Empty();
  private:
    TaskId Add(int qid, TaskFunc taskfunc);
    void Remove(int qid, TaskId taskid);
    void Execute(int qid, int tasknum);
  private:
    using TaskEntry = std::pair<TaskId, TaskFunc>;
    using LOCK_GUARD = std::lock_guard<std::mutex>;
    typedef struct TaskQueue {
        std::mutex lock;
        std::deque<TaskEntry> tasks;
    } TASK_QUEUE;

    static const int MAX_TASK_EXECUTED = 16;
    static const int MAX_TASK_QUEUE = 64;

    static const int INTEGRAL_BITS = sizeof(uint32_t) / sizeof(char) * 8;
  private:
    EventLoop* event_loop_;
    std::array<TASK_QUEUE, MAX_TASK_QUEUE> task_queues_;
    std::array<uint32_t, MAX_TASK_QUEUE / INTEGRAL_BITS> bit_masks_;
};
}
