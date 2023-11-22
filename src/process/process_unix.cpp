// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifndef _WIN32
#include "net/event_loop.h"
#include "base/error_code.h"
#include "process_unix.h"
#include "util/string_utils.h"
#include "process_event_handler.h"
#include "logging/logging.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>

namespace tinynet {
namespace process {

static const int kMaxWaitPidTimes = 20;

static const int kMaxArgumentsLength = 128 * 1024;

ProcessUnix::ProcessUnix(EventLoop* loop, ProcessEventHandler* handler) :
    ProcessImpl(loop, handler),
    pid_(0),
    wait_handle_(INVALID_TASK_ID),
    exit_code_(0),
    term_signal_(0) {
}

ProcessUnix::~ProcessUnix() {
    Dispose();
}

int ProcessUnix::Spawn(const ProcessOptions& options) {
    if (wait_handle_ != INVALID_TASK_ID || pid_ > 0) {
        return ERROR_PROCESS_SPAWN;
    }
    pid_t pid = fork();
    if (pid == 0) {
        std::string buf(kMaxArgumentsLength, 0);
        std::vector<char*> args;
        char* p = &buf[0];
        int size = 0;
        int total = 0;
        for (auto& arg : options.args) {
            size = arg.length() + 1;
            total += size;
            if (total > kMaxArgumentsLength) {
                break;
            }
            strncpy(p, arg.c_str(), size);
            args.push_back(p);
            p += size;
        }
        args.push_back(NULL);
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        execvp(options.file.c_str(), &args[0]);
        _exit(1);
        return 0;
    } else if (pid == -1) {
        return ERROR_PROCESS_SPAWN;
    }
    pid_ = pid;
    if (Wait(1) <= 0) {
        return ERROR_PROCESS_SPAWN;
    }
    wait_handle_ = event_loop_->AddSignal(SIGCHLD, std::bind(&ProcessUnix::OnSignal, this));
    return 0;
}

void ProcessUnix::Close() {
    Dispose();
}

void ProcessUnix::Dispose() {
    if (wait_handle_ != INVALID_TASK_ID) {
        event_loop_->ClearSignal(SIGCHLD, wait_handle_);
    }
    if (pid_ > 0) {
        int pid, status;
        status = 0;
        for (int i = 0; i < kMaxWaitPidTimes; ++i) {
            pid = waitpid(pid_, &status, WNOHANG);
            if (pid == 0) {
                ::kill(pid_, i == 0 ? SIGTERM : SIGKILL);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            } else if (pid == -1) {
                if (errno == EINTR) continue;
                else break;
            } else {
                break;
            }
        }
        pid_ = 0;
        exit_code_ = 0;
        term_signal_ = 0;
    }
}

int ProcessUnix::Kill(int signum) {
    if (wait_handle_ == INVALID_TASK_ID || pid_ <= 0) {
        return ERROR_PROCESS_KILL;
    }
    if (::kill(pid_, signum) == -1) {
        return ERROR_PROCESS_KILL;
    }
    return 0;
}

void ProcessUnix::OnSignal() {
    if (Wait(kMaxWaitPidTimes) == 0 && event_handler_)
        event_handler_->HandleExit(exit_code_, term_signal_);

}

int ProcessUnix::GetPid() {
    return static_cast<int>(pid_);
}

int ProcessUnix::Wait(int terms) {
    if (pid_ == 0) return -1;
    int pid, status;
    int i = 0;
    for (; i < terms; ++i) {
        status = 0;
        pid = waitpid(pid_, &status, WNOHANG);
        if (pid == 0) {
            if (i > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        } else if (pid == -1) {
            if (errno == EINTR) continue;
            else return -1;
        } else {
            exit_code_ = 0;
            term_signal_ = 0;
            if (WIFEXITED(status)) {
                exit_code_ = WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                term_signal_ = WTERMSIG(status);
            }
            pid_ = 0;
            return 0;
        }
    }
    return 1;
}
}
}
#endif
