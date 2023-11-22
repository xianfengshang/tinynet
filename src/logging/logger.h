// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#ifdef ERROR
#define  GLOG_NO_ABBREVIATED_SEVERITIES
#endif
#include "base/singleton.h"
#include <cstdarg>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>

namespace tinynet {
class EventLoop;
namespace rpc {
class RpcServer;
}

namespace logging {
struct LoggingContext;
class LoggingService;
class LoggerClient;
class LogDestination;

class Logger :
    public tinynet::Singleton<Logger> {
  public:
    Logger();
    ~Logger();
  public:
    int Init(const std::string& progname, const std::string& path, bool daemon = false);
    void Shutdown();
  public:
    void Info(const char *file, int line, const char *fmt, ...);
    void Warning(const char *file, int line, const char *fmt, ...);
    void Error(const char *file, int line, const char *fmt, ...);
    void Fatal(const char *file, int line, const char *fmt, ...);
    void LogFormat(const char *file, int line, int logLevel, const char* fmt, va_list args);
    void Log(const char *file, int line, int logLevel, const char* data, size_t len);
    void Log(const char* data, size_t len);

    void Flush();

    void WriteFileLog(const char *basename, const char* data, size_t len);

    void FlushFileLog(const char* basename);
  private:
    void RunActions();
  public:
    bool IsServerMode();
  public:
    void LogToFile(const char* file, int line, int logLevel, const std::string& msg);
    void LogToFile(const std::string& basename, const char* file, int line, int logLevel, const std::string& msg);
    void FlushLogFile(int level);
    void FlushLogFile(const std::string& base_filename);

    //customize log header
    //format: [Date HOST ProcessId ThreadId File:Line LEVEL]
    void FormatHeader(std::string& msg, const char* file, int line, int logLevel);
  public:
    const std::string& get_path() const { return path_; }

    const std::string& get_pipe_name() const { return pipe_name_; }
  private:
    void NotifyLog();
    void NotifyExit();
  public:
    void set_logtostderr(bool value);
    void set_logbufsecs(int value);
    void set_minloglevel(int value);
    void set_max_log_size(int value);
  private:
    std::string path_;
    std::string pipe_name_;
    std::string prog_name_;
    std::string prog_root_;
    std::unique_ptr<EventLoop> event_loop_;
    std::unique_ptr<LoggerClient> client_;
    std::unique_ptr<rpc::RpcServer> server_;
    std::unique_ptr<LoggingService> service_;
    std::unique_ptr<LogDestination> destination_;
    std::unique_ptr<std::thread> thread_;
    std::queue<std::shared_ptr<LoggingContext> > log_queue_;
    std::mutex lock_;
    std::string pid_;
    std::string host_;
    bool daemon_{ false };
    bool server_mode_{ false };
};
}
}
#define g_Logger tinynet::logging::Logger::Instance()
