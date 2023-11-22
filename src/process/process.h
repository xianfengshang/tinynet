// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/event_loop.h"
#include "process_event_handler.h"
#include "process_impl.h"
#include <memory>

namespace tinynet {
namespace process {
class Process {
  public:
    Process(EventLoop* loop, ProcessEventHandler* handler);
    ~Process();
  public:
    int Init();
    int Spawn(const ProcessOptions& options);
    int Kill(int signum);
    void Close();
    int GetPid();
  private:
    std::unique_ptr<ProcessImpl> impl_;
};
}
}