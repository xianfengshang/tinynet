// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#ifdef _WIN32
#include "base/error_code.h"
#include "process_win.h"
#include "util/string_utils.h"
#include "process_event_handler.h"
#include "base/at_exit.h"
#include "logging/logging.h"
#include <algorithm>
namespace tinynet {
namespace process {

static const int kMaxEnvironmentLength = 128 * 1024;

HANDLE ProcessWin::JOB_OBJECT_HANDLE = NULL;
std::once_flag ProcessWin::once_flag_;

void ProcessWin::Initialize() {
    SECURITY_ATTRIBUTES attr;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    memset(&attr, 0, sizeof(attr));
    attr.bInheritHandle = FALSE;
    memset(&info, 0, sizeof(info));
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK |
                                            JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK |
                                            JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION |
                                            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    JOB_OBJECT_HANDLE = CreateJobObjectW(&attr, NULL);
    if (JOB_OBJECT_HANDLE == NULL) {
        log_error("CreateJobObjectW failed, GetLastError()=%d", GetLastError());
        return;
    }
    if (!SetInformationJobObject(JOB_OBJECT_HANDLE, JobObjectExtendedLimitInformation, &info, sizeof(info))) {
        CloseHandle(JOB_OBJECT_HANDLE);
        JOB_OBJECT_HANDLE = NULL;
        log_error("SetInformationJobObject failed, GetLastError()=%d", GetLastError());
        return;
    }
    tinynet::AtExitManager::RegisterCallback(Finalize, nullptr);
}

void ProcessWin::Finalize(void*) {
    if (JOB_OBJECT_HANDLE != NULL) {
        CloseHandle(JOB_OBJECT_HANDLE);
        JOB_OBJECT_HANDLE = NULL;
    }
}

ProcessWin::ProcessWin(EventLoop* loop, ProcessEventHandler* handler) :
    ProcessImpl(loop, handler),
    pid_(0),
    process_handle_(INVALID_HANDLE_VALUE),
    wait_handle_(INVALID_HANDLE_VALUE),
    exit_code_(0) {
    std::call_once(once_flag_, Initialize);
}

ProcessWin::~ProcessWin() {
    Dispose();
}

int ProcessWin::Spawn(const ProcessOptions& options) {
    if (wait_handle_ != INVALID_HANDLE_VALUE || process_handle_ != INVALID_HANDLE_VALUE) {
        return ERROR_PROCESS_SPAWN;
    }
    //command line arguments
    std::string cmd = StringUtils::join(options.args, " ");
    std::wstring wcmd;
    StringUtils::convert_utf8_to_utf16(cmd, &wcmd);

    //environ
    std::vector<std::wstring> environ_strings;
    //copy parent's environ
    LPWCH pEnviron = GetEnvironmentStringsW();
    LPWSTR pVariable = (LPWSTR)pEnviron;
    int size = 0;
    while (*pVariable) {
        size = (int)wcslen(pVariable) + 1;
        environ_strings.emplace_back(pVariable);
        pVariable += size;
    }
    for (auto& env : options.env) {
        std::string var;
        std::wstring wvar;
        StringUtils::Format(var, "%s=%s", env.first.c_str(), env.second.c_str());
        if (StringUtils::convert_utf8_to_utf16(var, &wvar)) {
            environ_strings.emplace_back(std::move(wvar));
        }
    }
    std::sort(environ_strings.begin(), environ_strings.end());

    std::wstring buf(kMaxEnvironmentLength, 0);
    wchar_t* p = &buf[0];
    int total = 0;
    for (auto& env : environ_strings) {
        size = (int)env.length() + 1;
        total += size;
        if (total > kMaxEnvironmentLength) {
            break;
        }
        wcsncpy(p, env.c_str(), size);
        p += size;
    }
    *p = (wchar_t)0;

    //creation flags
    DWORD flags = 0;
    if (options.flags & PROCESS_FLAGS_NEW_CONSOLE) flags |= CREATE_NEW_CONSOLE;
    if (options.flags & PROCESS_FLAGS_NO_WINDOW) flags |= CREATE_NO_WINDOW;
    if (options.flags & PROCESS_FLAGS_DETACHED_PROCESS) flags |= DETACHED_PROCESS;
    if (options.flags & PROCESS_FLAGS_UNICODE_ENVIRONMENT) flags |= CREATE_UNICODE_ENVIRONMENT;

    STARTUPINFOW si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    BOOL res = CreateProcessW(NULL, &wcmd[0], NULL, NULL, FALSE, flags, &buf[0], NULL, &si, &pi);
    if (!res) {
        log_error("CreateProcessW failed, GetLastError()=%d", GetLastError());
        return ERROR_PROCESS_SPAWN;
    }
    CloseHandle(pi.hThread);
    process_handle_ = pi.hProcess;
    pid_ = pi.dwProcessId;
    res = RegisterWaitForSingleObject(&wait_handle_, process_handle_, handle_wait, this, INFINITE, WT_EXECUTEONLYONCE);
    if (!res) {
        Dispose();
        return ERROR_PROCESS_SPAWN;
    }
    if (JOB_OBJECT_HANDLE != NULL && AssignProcessToJobObject(JOB_OBJECT_HANDLE, process_handle_) == FALSE) {
        if (GetLastError() != ERROR_ACCESS_DENIED) {
            log_error("AssignProcessToJobObject failed, GetLastError()=%d", GetLastError());
        }

    }
    return 0;
}

void ProcessWin::Close() {
    Dispose();
}

int ProcessWin::Kill(int signum) {
    if (wait_handle_ == INVALID_HANDLE_VALUE  || process_handle_ == INVALID_HANDLE_VALUE) {
        return ERROR_PROCESS_KILL;
    }
    if (TerminateProcess(process_handle_, -1) == FALSE) {
        return ERROR_PROCESS_KILL;
    }
    return 0;
}

void ProcessWin::Dispose() {
    if (wait_handle_ != INVALID_HANDLE_VALUE) {
        UnregisterWait(wait_handle_);
        wait_handle_ = INVALID_HANDLE_VALUE;
    }
    if (process_handle_ != INVALID_HANDLE_VALUE) {
        if (GetExitCodeProcess(process_handle_, &exit_code_) && exit_code_ == STILL_ACTIVE) {
            exit_code_ = 0;
            if (TerminateProcess(process_handle_, -1)) {
                if (WaitForSingleObject(process_handle_, 100) == WAIT_OBJECT_0) {
                    GetExitCodeProcess(process_handle_, &exit_code_);
                }
            }
        }
        CloseHandle(process_handle_);
        process_handle_ = INVALID_HANDLE_VALUE;
    }
}

void ProcessWin::OnExit() {
    if (event_handler_) {
        event_handler_->HandleExit(exit_code_, 0);
    }
}

void CALLBACK ProcessWin::handle_wait(PVOID lpParameter, BOOLEAN timerOrWaitFired) {
    ProcessWin* self = static_cast<ProcessWin*>(lpParameter);
    if (!self) return;
    self->OnExit();
}

int ProcessWin::GetPid() {
    return static_cast<int>(pid_);
}

}
}
#endif
