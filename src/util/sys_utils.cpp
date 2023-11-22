// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <stdio.h>
#include <string.h>
#include <string>
#if defined(_WIN32)
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#endif
namespace SysUtils {
#ifdef __linux__
bool transparent_hugepage_is_enabled() {
    char buf[1024] = { 0 };
    FILE* fp = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r");
    if (!fp) {
        return 0;
    }
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    //Possible output is [always] madvise never
    return strstr(buf, "[never]") == NULL;
}

static const size_t kRlimAdjustStep = 16;

bool adjust_open_files_limit(size_t expected_value) {
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
        return false;
    }
    if (limit.rlim_cur >= expected_value) {
        return true;
    }
    rlim_t old_value = limit.rlim_cur;
    rlim_t new_value = expected_value;
    while (new_value > old_value) {
        limit.rlim_cur = new_value;
        limit.rlim_max = new_value;
        if (setrlimit(RLIMIT_NOFILE, &limit) == 0) break;
        if (new_value <= kRlimAdjustStep) break;
        new_value -= 16;
    }
    return true;
}

#endif

int get_cpu_count() {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
#else
    return get_nprocs();
#endif
}

const char* get_os_name() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(macintosh)
    return "MacOS";
#elif defined(__ANDROID__)
    return "Android";
#elif defined(__gnu_linux__)
    return "GNU Linux";
#elif defined(__sun)
    return "Solaris";
#elif defined(__FreeBSD__)
    return "FreeBSD";
#elif defined(__OpenBSD__)
    return "OpenBSD";
#else
    return "Unknown";
#endif
}

std::string get_host_name() {
    char buf[1024];
    if (gethostname(buf, sizeof(buf)) == 0) {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    return "unknown";
}
}
