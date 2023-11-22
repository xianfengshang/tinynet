// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "singleton.h"
#include <memory>
#include <stdio.h>
#include "file_lock.h"

namespace tinynet {
class RuntimeLogger:
    public tinynet::Singleton<RuntimeLogger> {
  public:
    RuntimeLogger();
    ~RuntimeLogger();
  public:
    void Error(const char *file, int line, const char* fmt, ...);
    void Fatal(const char *file, int line, const char* fmt, ...);
    void Log(const char *file, int line, const char* data);
    void Log(const char *file, int line, const char* fmt, va_list args);
  private:
    void FormatHeader(std::string& msg, const char* file, int line);
  private:
    FILE* fp_;
    FileLock lock_;
    int pid_;
};
}

#define g_RuntimeLogger tinynet::RuntimeLogger::Instance()

#define log_runtime_error(fmt, ...) g_RuntimeLogger->Error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
