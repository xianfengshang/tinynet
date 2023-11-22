// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "singleton.h"
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace tinynet {
enum class ConsoleColor {
    Default,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
};

enum class ConsoleLogMode {
    Sync,
    Async
};

class ConsoleLogger:
    public tinynet::Singleton<ConsoleLogger> {
  public:
    ConsoleLogger();
    ~ConsoleLogger();
  public:
    void Init(ConsoleLogMode mode = ConsoleLogMode::Sync);
    void Shutdown();
    void Log(const char* data, size_t len, ConsoleColor color = ConsoleColor::Default);
  private:
    void Run() noexcept;
  private:
    struct LogEntry {
        ConsoleColor color;
        std::string data;
    };
  private:
    std::atomic<ConsoleLogMode> mode_;
    std::atomic_bool stop_;

    std::queue<std::shared_ptr<LogEntry> > queue_;
    std::unique_ptr<std::thread> thread_;
    std::mutex lock_;
    std::condition_variable cv_;
};
}

#define g_ConsoleLogger tinynet::ConsoleLogger::Instance()
