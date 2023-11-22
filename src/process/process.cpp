// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "process.h"
#ifdef _WIN32
#include "process_win.h"
#else
#include "process_unix.h"
#endif

namespace tinynet {
namespace process {
Process::Process(EventLoop* loop, ProcessEventHandler* handler) {
#ifdef _WIN32
    impl_.reset(new(std::nothrow) ProcessWin(loop, handler));
#else
    impl_.reset(new(std::nothrow) ProcessUnix(loop, handler));
#endif

}

Process::~Process() = default;

int Process::Init() {
    return 0;
}

int Process::Spawn(const ProcessOptions& options) {
    return impl_->Spawn(options);
}

void Process::Close() {
    return impl_->Close();
}

int Process::Kill(int signum) {
    return impl_->Kill(signum);
}

int Process::GetPid() {
    return impl_->GetPid();
}
}
}