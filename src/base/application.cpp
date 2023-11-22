// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "application.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <exception>
#include <stdio.h>
#include "clock.h"
#include "module.h"
#include <iostream>
#include <csignal>
#include "cmdline.h"
#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#define unlink _unlink
#else
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <clocale>
#endif
#include "runtime_logger.h"
#include "util/fs_utils.h"

#ifdef _WIN32
LONG __stdcall ExceptionHandler(EXCEPTION_POINTERS* pep) {
    DWORD dwProcessId;
    HANDLE hFile;
    char szFileName[MAX_PATH];
    char szModuleName[MAX_PATH];
    char szTimeBuffer[64];
    struct tm *dt;
    time_t curTime;

    GetModuleFileName(NULL, szModuleName, MAX_PATH);
    curTime = time(NULL);
    dt = localtime(&curTime);
    strftime(szTimeBuffer, sizeof(szTimeBuffer), "%Y-%m-%d-%H-%M-%S", dt);
    dwProcessId = GetCurrentProcessId();

    snprintf(szFileName, sizeof(szFileName), "%s-%s-%d.dmp", szModuleName, szTimeBuffer, dwProcessId);
    hFile = CreateFileA(szFileName, GENERIC_READ | GENERIC_WRITE,
                        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile != INVALID_HANDLE_VALUE)) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;

        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pep;
        mdei.ClientPointers = FALSE;

        MINIDUMP_TYPE mdt = MiniDumpNormal;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                          hFile, mdt, (pep != 0) ? &mdei : 0, 0, 0);

        CloseHandle(hFile);

    }
    return EXCEPTION_EXECUTE_HANDLER;
}

static HANDLE SHUTDOWN_EVENT = INVALID_HANDLE_VALUE;

BOOL CALLBACK CosonleHandler(DWORD ev) {
    BOOL bRet = FALSE;
    switch (ev) {
    case CTRL_CLOSE_EVENT:
        SHUTDOWN_EVENT = CreateEvent(NULL, FALSE, FALSE, NULL);
        g_App->Shutdown();
        if (SHUTDOWN_EVENT != NULL) {
            WaitForSingleObject(SHUTDOWN_EVENT, INFINITE);
            CloseHandle(SHUTDOWN_EVENT);
            SHUTDOWN_EVENT = INVALID_HANDLE_VALUE;
        }
        bRet = TRUE;
        break;
    default:
        break;
    }
    return bRet;
}
#else
extern char **environ;
#endif

tinynet::Application app;

tinynet::Application* g_App = &app;

namespace tinynet {

ModuleEntries& GetModuleEntries() {
    static ModuleEntries s_entries;
    return s_entries;
}


Application::Application() :
    shutdown_(false),
    daemonize_(false),
    argv_space_size_(0) {
}

int Application::Init() {
#ifndef _WIN32
    std::setlocale(LC_ALL, "en_US.utf8");
#endif
    ParseCommandLine();
    Daemonize();
    InstallSignalHandlers();
    MigrateEnviron();

    for (auto& entry : GetModuleEntries()) entry();

    modules_.sort([](const ModuleItem& a, const ModuleItem& b) {
        return a.priority > b.priority;
    });

    int ret;
    for (auto& module : modules_) {
        if (module.Init && (ret = module.Init()) != 0) {
            log_runtime_error("Module %s Init() failed, ret: %d", module.module->get_name(), ret);
            return 1;
        }
    }
    return 0;
}

int Application::Start() {
    int ret;
    for (auto& module : modules_) {
        if (module.Start && (ret = module.Start()) != 0) {
            log_runtime_error("Module %s Start() failed, ret: %d", module.module->get_name(), ret);
            return 1;
        }
    }
    return 0;
}

int Application::Stop() {
    for (auto it = modules_.rbegin();
            it != modules_.rend(); ++it) {
        auto &module = *it;
        if (module.Stop) {
            try {
                module.Stop();
            } catch (std::exception& e) {
                log_runtime_error("Module %s Stop() exception: %s", module.module->get_name(), e.what());
            }
        }
    }
    if (daemonize_) {
        unlink(pidfile_.c_str());
    }
#ifdef _WIN32
    if (SHUTDOWN_EVENT != INVALID_HANDLE_VALUE) {
        SetEvent(SHUTDOWN_EVENT);
    }
#endif
    return 0;
}

int Application::Run() {
    int ret;
    for (auto& module : modules_) {
        if (module.Run && (ret = module.Run()) != 0) {
            log_runtime_error("Module %s Run() failed, ret: %d", module.module->get_name(), ret);
            return 1;
        }
    }
    return 0;
}

int Application::Signal(int signum) {
    int ret;
    for (auto& module : modules_) {
        if (module.Signal && (ret = module.Signal(signum)) != 0) {
            log_runtime_error("Module %s Signal(%d) failed, ret: %d", module.module->get_name(), signum, ret);
        }
    }
    return 0;
}

int Application::RunForever() {
    int code;
    for (;;) {
        if (shutdown_) break;
        if ((code = Run()) != 0) return code;
    }
    return 0;
}

bool Application::RegisterModule(const ModuleItem& item) {
    ModuleItemList::iterator it = std::find(modules_.begin(), modules_.end(), item);
    if (it != modules_.end()) return false;
    modules_.push_back(item);
    return true;
}

void Application::ParseCommandLine() {
    tinynet::cmdline::Parser cmd_parser;
    cmd_parser.add("pidfile", 'p', "specify pid file path", tinynet::cmdline::Parser::OptionType::kv,
                   tinynet::cmdline::Parser::OptionRule::optional);
    cmd_parser.add("daemonize", 'd', "daemonize the program", tinynet::cmdline::Parser::OptionType::kv,
                   tinynet::cmdline::Parser::OptionRule::optional);
    cmd_parser.parse_check(args_);
    daemonize_ = cmd_parser.has("daemonize");
    if (cmd_parser.has("pidfile")) {
        pidfile_ = cmd_parser.get("pidfile");
    } else {
        pidfile_ = args_[0] + ".pid";
    }
}

struct SignalInfo {
    const char* name;
    int number;
};

static SignalInfo shutdown_signals[] = {
    {"SIGABRT", SIGABRT},
    {"SIGTERM",SIGTERM },
    {"SIGINT",SIGINT },
    {0, 0}
};

#ifdef _WIN32
void ShupdownHandler(int signum) {
    g_App->Shutdown();
}

void SignalHandler(int signum) {
    g_App->Signal(signum);
}

#else

static SignalInfo interest_signals[] = {
    {"SIGHUP", SIGHUP},
    {"SIGUSR1", SIGUSR1},
    {"SIGUSR2", SIGUSR2},
    {"SIGCHLD", SIGCHLD},
    {0, 0}
};

void ShupdownHandler(int signum,
                     siginfo_t *signal_info,
                     void *ucontext) {
    g_App->Shutdown();
}
void SignalHandler(int signum,
                   siginfo_t *signal_info,
                   void *ucontext) {
    g_App->Signal(signum);
}

#endif

void Application::InstallSignalHandlers() {
#ifdef _WIN32
    SetUnhandledExceptionFilter(ExceptionHandler);
    SetConsoleCtrlHandler(CosonleHandler,TRUE);
    SignalInfo* info;
    for (info = shutdown_signals; info->name; info++) {
        signal(info->number, ShupdownHandler);
    }
#else
    signal(SIGPIPE, SIG_IGN);
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_sigaction = &ShupdownHandler;

    SignalInfo* info;
    for (info = shutdown_signals; info->name; info++) {
        sigaction(info->number, &sig_action, NULL);
    }
    sig_action.sa_sigaction = &SignalHandler;
    for (info = interest_signals; info->name; info++) {
        sigaction(info->number, &sig_action, NULL);
    }
#endif
}

void Application::CreatePidFile() {
    //Make sure directory exist
    std::string path = FileSystemUtils::basedir(pidfile_);
    if (!FileSystemUtils::exists(path)) {
        FileSystemUtils::create_directories(path);
    }

    FILE *fp = fopen(pidfile_.c_str(), "w");
    if (fp) {
#ifdef _WIN32
        fprintf(fp, "%d\n", (int)GetCurrentProcessId());
#else
        fprintf(fp, "%d\n", (int)getpid());
#endif
        fclose(fp);
    }
}

void Application::Daemonize() {
    if (daemonize_) {
#ifndef _WIN32
        int fd;

        if (fork() != 0) exit(0); /* parent exits */
        setsid(); /* create a new session */

        if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > STDERR_FILENO) close(fd);
        }
#endif
        CreatePidFile();
    }
}

void Application::SetArgs(int argc, char* argv[]) {
    cmd_line_args_.argc = argc;
    cmd_line_args_.argv = argv;
    for (int i = 0; i < argc; ++i) {
        args_.push_back(argv[i]);
    }
}

void Application::Shutdown() {
    shutdown_ = true;
}

#ifdef __linux__
void Application::MigrateEnviron() {
    char* argv_last = cmd_line_args_.argv[0];
    for (int i = 0; i < cmd_line_args_.argc; ++i) {
        if (argv_last == cmd_line_args_.argv[i]) {
            argv_last = cmd_line_args_.argv[i] + strlen(cmd_line_args_.argv[i]) + 1;
        }
    }
    size_t size = 0;
    for (size_t i = 0; environ[i]; i++) {
        size += strlen(environ[i]) + 1;
    }
    argv_space_size_ = argv_last - cmd_line_args_.argv[0];
    migrated_environ_.reset(new(std::nothrow) char[size]);
    if (!migrated_environ_) return;
    char* p = migrated_environ_.get();
    for (size_t i = 0; environ[i]; i++) {
        if (argv_last == environ[i]) {
            size = strlen(environ[i]) + 1;
            argv_last = environ[i] + size;

            strncpy(p, environ[i], size);
            environ[i] = p;
            p += size;
        }
    }
    argv_space_size_ = argv_last - cmd_line_args_.argv[0];
}
#else
void Application::MigrateEnviron() {
}
#endif
}
