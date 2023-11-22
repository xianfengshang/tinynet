// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "runtime_logger.h"
#include "clock.h"
#include "util/string_utils.h"
#include "util/process_utils.h"
#include "util/date_utils.h"
#include "util/fs_utils.h"
#include "application.h"
#include <string.h>

namespace tinynet {

static const char* LOG_NAMES[] = {
    "runtime_error.log",
    "runtime_error2.log",
    "runtime_error3.log",
};

RuntimeLogger::RuntimeLogger():
    fp_(NULL),
    pid_(0) {
    for (size_t i = 0; i < sizeof(LOG_NAMES) / sizeof(LOG_NAMES[0]); ++i) {
        fp_ = fopen(LOG_NAMES[i], "ab");
        if (fp_ != NULL) break;
    }
    if (fp_ != NULL) {
        int fd;
#ifdef _MSC_VER
        fd = _fileno(fp_);
#else
        fd = fileno(fp_);
#endif
        lock_.init(fd);
    } else {
        fp_ = stderr;
    }
    pid_ = ProcessUtils::get_pid();
}

RuntimeLogger::~RuntimeLogger() {
    if (fp_ && fp_ != stderr) {
        fclose(fp_);
        fp_ = NULL;
    }
}

void RuntimeLogger::Error(const char* file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log(file, line, fmt, args);
    va_end(args);
}

void RuntimeLogger::Fatal(const char* file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log(file, line, fmt, args);
    va_end(args);
    g_App->Shutdown();
}

void RuntimeLogger::Log(const char *file, int line, const char* fmt, va_list args) {
    std::string msg;
    FormatHeader(msg, file, line);
    StringUtils::VFormat(msg, fmt, args);
    msg.append("\n");

    std::lock_guard<FileLock> lock(lock_);
    fwrite(&msg[0], 1, msg.size(), fp_);
    fflush(fp_);
}

void RuntimeLogger::Log(const char *file, int line, const char* data) {
    std::string msg;
    FormatHeader(msg, file, line);
    msg.append(data);
    msg.append("\n");

    std::lock_guard<FileLock> lock(lock_);
    fwrite(&msg[0], 1, msg.size(), fp_);
    fflush(fp_);
}

void RuntimeLogger::FormatHeader(std::string& msg, const char* file, int line) {
    std::string date_string = DateUtils::NowDateString();
    int tid = ProcessUtils::get_tid();
    char buf[256];
    FileSystemUtils::basename(file, buf, sizeof(buf));
    StringUtils::Format(msg, "[%s %d:%d %s:%d] ", date_string.c_str(), pid_, tid, buf, line);
}
}
