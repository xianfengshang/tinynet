// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef _WIN32
#pragma once
#include "process_impl.h"
#include <Windows.h>

namespace tinynet {
class EventLoop;
namespace process {
class ProcessEventHandler;
class ProcessWin : public ProcessImpl {
  public:
    ProcessWin(EventLoop* loop, ProcessEventHandler* handler);
    ~ProcessWin();
  public:
    int Spawn(const ProcessOptions& options) override;
    void Close() override;
    int Kill(int signum) override;
    int GetPid() override;
  private:
    void Dispose();
    void OnExit();
  private:
    static std::once_flag once_flag_;
    static HANDLE JOB_OBJECT_HANDLE;
    static void Initialize();
    static void Finalize(void*);
    static void CALLBACK handle_wait(PVOID lpParameter, BOOLEAN timerOrWaitFired);
  private:
    DWORD pid_;
    HANDLE process_handle_;
    HANDLE wait_handle_;
    DWORD exit_code_;
};
}
}
#endif
