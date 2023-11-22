// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <string.h>
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#include "string_utils.h"
#else
#include "base/application.h"
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)
#endif
namespace ProcessUtils {

#ifdef _WIN32
int get_pid() {
    return static_cast<int>(GetCurrentProcessId());
}
int get_tid() {
    return static_cast<int>(GetCurrentThreadId());
}

void set_process_title(const char* title) {
    std::wstring u16_title;
    StringUtils::convert_utf8_to_utf16(title, &u16_title);
    SetConsoleTitleW(u16_title.c_str());
}
#else
int get_pid() {
    return getpid();
}
int get_tid() {
    return gettid();
}
void set_process_title(const char* title) {
    char* dst = g_App->GetCommandLineArgs().argv[0];
    strncpy(dst, title, g_App->get_argv_space_size());
    dst[g_App->get_argv_space_size() - 1] = '\0';
}
#endif
}

