// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifndef _WIN32
#pragma once
#include <unistd.h>
#include "process_impl.h"

namespace tinynet {
class EventLoop;
namespace process {
class ProcessEventHandler;
class ProcessUnix : public ProcessImpl {
  public:
    ProcessUnix(EventLoop* loop, ProcessEventHandler* handler);
    ~ProcessUnix();
  public:
    int Spawn(const ProcessOptions& options) override;
    void Close() override;
    int Kill(int signum) override;
    int GetPid() override;
  private:
    void Dispose();
    void OnSignal();
    int Wait(int terms);
  private:
    pid_t pid_;
    int64_t wait_handle_;
    int exit_code_;
    int term_signal_;
};
}
}
#endif
