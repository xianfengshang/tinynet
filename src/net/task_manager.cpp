// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "task_manager.h"
#include "event_loop.h"
#include <algorithm>

namespace tinynet {

TaskManager::TaskManager(EventLoop* loop):
    event_loop_(loop) {
}

TaskManager::~TaskManager() = default;

void TaskManager::Init() {
    for (size_t i = 0; i < bit_masks_.size(); ++i) {
        bit_masks_[i] = 0;
    }
}

void TaskManager::Stop() {
    for (size_t i = 0; i < MAX_TASK_QUEUE; ++i) {
        LOCK_GUARD lock(task_queues_[i].lock);
        task_queues_[i].tasks.clear();
    }
}

void TaskManager::Run() {
    Execute(0, MAX_TASK_EXECUTED);
    for (int i = 1; i < MAX_TASK_QUEUE; ++i) {
        if ((bit_masks_[i / INTEGRAL_BITS] & (1 << (i % INTEGRAL_BITS)))) {
            {
                LOCK_GUARD lock(task_queues_[i].lock);
                bit_masks_[i / INTEGRAL_BITS] &= ~(1 << (i % INTEGRAL_BITS));
            }
            Execute(i, 0);
        }
    }
}

TaskId TaskManager::AddTask(TaskFunc taskfunc) {
    return Add(0, taskfunc);
}

void TaskManager::CancelTask(TaskId& taskId) {
    Remove(0, taskId);
    taskId = INVALID_TASK_ID;
}

TaskId TaskManager::AddSignal(int signum, TaskFunc sigfunc) {
    return Add(signum, sigfunc);
}

void TaskManager::ClearSignal(int signum, TaskId& sigtid) {
    Remove(signum, sigtid);
    sigtid = INVALID_TASK_ID;
}

TaskId TaskManager::Add(int qid, TaskFunc taskfunc) {
    if (qid < 0 || qid >= MAX_TASK_QUEUE) {
        return INVALID_TASK_ID;
    }
    if (!taskfunc) return INVALID_TASK_ID;
    TaskId taskid = event_loop_->NewUniqueId();
    TASK_QUEUE& task_queue = task_queues_[qid];
    {
        std::lock_guard<std::mutex> lock(task_queue.lock);
        task_queue.tasks.emplace_back(std::make_pair(taskid, taskfunc));
    }
    return taskid;
}

void TaskManager::Remove(int qid, TaskId taskid) {
    if (qid < 0 || qid >= MAX_TASK_QUEUE) {
        return;
    }
    TASK_QUEUE& task_queue = task_queues_[qid];
    {
        LOCK_GUARD lock(task_queue.lock);
        for (auto& it : task_queue.tasks) {
            if (it.first == taskid) {
                it.second = nullptr;
                break;
            }
        }
    }
}

void TaskManager::Execute(int qid, int tasknum) {
    if (qid < 0 || qid >= MAX_TASK_QUEUE) {
        return;
    }
    TASK_QUEUE& task_queue = task_queues_[qid];
    if (task_queue.tasks.empty()) return;
    std::deque<TaskEntry> tasks;
    {
        LOCK_GUARD lock(task_queue.lock);
        if (qid == 0) {
            if (tasknum > 0 && tasknum < (int)task_queue.tasks.size()) {
                for (int i = 0; i < tasknum; i++) {
                    tasks.push_back(task_queue.tasks[i]);
                }
                task_queue.tasks.erase(task_queue.tasks.begin(), task_queue.tasks.begin() + tasknum);
            } else {
                tasks.swap(task_queue.tasks);
            }
        } else {
            tasks.assign(task_queue.tasks.begin(), task_queue.tasks.end());
        }
    }

    for (auto& task : tasks) {
        Invoke(task.second);
    }
}

void TaskManager::Signal(int signum) {
    if (signum < 0 || signum >= MAX_TASK_QUEUE) {
        return;
    }
    TASK_QUEUE& task_queue = task_queues_[signum];
    {
        LOCK_GUARD lock(task_queue.lock);
        bit_masks_[signum / INTEGRAL_BITS] |= 1 << (signum % INTEGRAL_BITS);
    }
}

bool TaskManager::Empty() {
    for (size_t i = 0; i < bit_masks_.size(); ++i) {
        if (bit_masks_[i] != 0) {
            return false;
        }
    }
    return task_queues_[0].tasks.empty();
}
}
