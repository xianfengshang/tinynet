// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "console_logger.h"
#include "util/fs_utils.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace tinynet {

static size_t kMaxLogQueueSize = 9000;

#ifdef _WIN32
static WORD ColorToCode(ConsoleColor color) {
    switch (color) {
    case ConsoleColor::Default:
        return 0;
    case ConsoleColor::Red:
        return FOREGROUND_RED;
    case ConsoleColor::Green:
        return FOREGROUND_GREEN;
    case ConsoleColor::Yellow:
        return FOREGROUND_RED | FOREGROUND_GREEN;
    case ConsoleColor::Blue:
        return FOREGROUND_BLUE;
    case  ConsoleColor::Magenta:
        return  FOREGROUND_RED | FOREGROUND_BLUE;
    case ConsoleColor::Cyan:
        return FOREGROUND_GREEN | FOREGROUND_BLUE;
    case ConsoleColor::White:
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    default:
        return 0;
    }
}

struct ConsoleColorGuard {
    ConsoleColorGuard(ConsoleColor color) {
        stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO buffer_info;
        GetConsoleScreenBufferInfo(stderr_handle, &buffer_info);
        old_color_attrs = buffer_info.wAttributes;
        SetConsoleTextAttribute(stderr_handle, ColorToCode(color) | FOREGROUND_INTENSITY);
    }
    ~ConsoleColorGuard() {
        SetConsoleTextAttribute(stderr_handle, old_color_attrs);
    }
    HANDLE stderr_handle;
    WORD old_color_attrs;
};

#else
static const char* ColorToCode(ConsoleColor color) {
    switch (color) {
    case ConsoleColor::Default:
        return "0";
    case ConsoleColor::Red:
        return "1";
    case ConsoleColor::Green:
        return "2";
    case ConsoleColor::Yellow:
        return "3";
    case ConsoleColor::Blue:
        return "4";
    case  ConsoleColor::Magenta:
        return "5";
    case ConsoleColor::Cyan:
        return "6";
    case ConsoleColor::White:
        return "7";
    default:
        return "0";
    }
}

struct ConsoleColorGuard {
    ConsoleColorGuard(ConsoleColor color) {
        fprintf(stderr, "\033[0;3%sm", ColorToCode(color));
    }
    ~ConsoleColorGuard() {
        fprintf(stderr, "\033[0m");
        fflush(stderr);
    }
};
#endif

static void LogToStderr(const char* msg, size_t len, ConsoleColor color) {
    if (color == ConsoleColor::Default) {
        fwrite(msg, sizeof(char), len, stderr);
        return;
    }
    ConsoleColorGuard guard(color);
    fwrite(msg, sizeof(char), len, stderr);
}

ConsoleLogger::ConsoleLogger():
    mode_(ConsoleLogMode::Sync),
    stop_(false) {
}

ConsoleLogger::~ConsoleLogger() {
}

void ConsoleLogger::Init(ConsoleLogMode mode /*= ConsoleLogMode::Sync*/) {
    if (mode == ConsoleLogMode::Async) {
        thread_.reset(new(std::nothrow) std::thread(&ConsoleLogger::Run, this));
    }
    mode_ = mode;
}

void ConsoleLogger::Shutdown() {
    if (stop_) return;
    stop_ = true;
    if (mode_ == ConsoleLogMode::Async) {
        cv_.notify_all();
        thread_->join();
    }
}

void ConsoleLogger::Log(const char* data, size_t len, ConsoleColor color/* = ConsoleColor::Default*/) {
    if (stop_ || mode_ == ConsoleLogMode::Sync) {
        LogToStderr(data, len, color);
        return;
    }
    auto entry = std::make_shared<LogEntry>();
    entry->color = color;
    entry->data.assign(data, len);
    {
        std::lock_guard<std::mutex> lock(lock_);
        if (queue_.size() == kMaxLogQueueSize) {
            queue_.pop();
        }
        queue_.emplace(std::move(entry));
    }
    cv_.notify_one();
}



void ConsoleLogger::Run() noexcept {
    auto pred = [this]() { return stop_ || !queue_.empty(); };
    while (!stop_) {
        std::unique_lock<std::mutex> lock(lock_);
        cv_.wait(lock, pred);
        if (stop_) return;
        std::queue<std::shared_ptr<LogEntry> > logs;
        queue_.swap(logs);
        lock.unlock();

        while (logs.size() > 0) {
            auto entry = logs.front();
            logs.pop();
            LogToStderr(&entry->data[0], entry->data.length(), entry->color);
        }
    }
}
}
