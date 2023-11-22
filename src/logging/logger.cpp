// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "logging.h"
#include "base/application.h"
#include <cstdarg>
#include <stdio.h>
#include <chrono>
#include "base/runtime_logger.h"
#include "base/crypto.h"
#include "util/fs_utils.h"
#include "util/process_utils.h"
#include "util/date_utils.h"
#include "util/net_utils.h"
#include "util/string_utils.h"

#include "net/event_loop.h"
#include "logger_client.h"
#include "logging_service.h"
#include "log_destination.h"
#include "rpc/rpc_server.h"
#include "base/id_allocator.h"
#include "base/error_code.h"
#include "base/console_logger.h"

namespace tinynet {
namespace logging {

int FLAGS_minloglevel = 0;

static const int kLogSignal = 10;

static const int kExitSingal = 12;

static const char* LOG_LEVEL_NAMES[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
};

static_assert(sizeof(LOG_LEVEL_NAMES) / sizeof(LOG_LEVEL_NAMES[0]) == MAX_LOG_LEVELS, "LOG_LEVEL_NAMES does not match the definition of Logger::LogLevel");

Logger::Logger() = default;

Logger::~Logger() = default;

int Logger::Init(const std::string& progname, const std::string& path, bool daemon /* = false*/) {
    prog_name_ = progname;
    prog_root_ = FileSystemUtils::basedir(progname);
    path_ = path;
    daemon_ = daemon;

    server_mode_ = IsServerMode();

    // init vars
    NetUtils::GetLocalIP(&host_);
    pid_ = std::to_string(ProcessUtils::get_pid());

    // 1+ setup event loop
    event_loop_.reset(new(std::nothrow) EventLoop());
    event_loop_->Init();

    // 2+ setup id worker
    event_loop_->SetIdAllocator(tinynet::IdAllocator::Instance(), false);

    // 3+ install signal task
    event_loop_->AddSignal(kLogSignal, std::bind(&Logger::RunActions, this));
    event_loop_->AddSignal(kExitSingal, std::bind(&EventLoop::Exit, event_loop_.get()));

    if (server_mode_) {
        StringUtils::Format(pipe_name_, "%s/%s.sock", path_.c_str(),
                            FileSystemUtils::basename(progname, false).c_str());
    } else {
        pipe_name_ = path_;
    }

    // 3+ setup log server
    if (server_mode_) {
        server_.reset(new (std::nothrow) rpc::RpcServer(event_loop_.get()));
        service_.reset(new (std::nothrow) LoggingService(event_loop_.get()));
        destination_.reset(new (std::nothrow) LogDestination());
        if (!server_ || !service_ || !destination_) {
            return ERROR_OS_OOM;
        }
        service_->RegisterService(server_.get());
        destination_->Init(path_);


        FileSystemUtils::remove(pipe_name_);
        net::ServerOptions opts;
        opts.name = "Log";
        opts.listen_path = pipe_name_;
        int err = server_->Start(opts);
        if (err) {
            return err;
        }

        if (!daemon_) g_ConsoleLogger->Init(ConsoleLogMode::Async);
    }
    // 4+ setup log client
    client_.reset(new (std::nothrow) LoggerClient(event_loop_.get()));
    client_->Init(pipe_name_);

    // 5+ setup event loop thread
    thread_.reset(new(std::nothrow) std::thread(&tinynet::EventLoop::Run, event_loop_.get(), tinynet::EventLoop::RUN_FOREVER));
    if (!thread_) {
        return ERROR_OS_OOM;
    }
    return 0;
}

void Logger::Shutdown() {
    NotifyExit();

    thread_->join();

    client_->Stop();

    if (server_) {
        server_->Stop();
        FileSystemUtils::remove(pipe_name_);
    }

    event_loop_->Stop();

    event_loop_->Run(EventLoop::RUN_NOWAIT);

    if (server_mode_) {
        if (destination_) {
            destination_->FlushAll();
        }

        if (!daemon_) {
            g_ConsoleLogger->Shutdown();
        }
    }
}

void Logger::LogFormat(const char *file, int line, int logLevel, const char* fmt, va_list args) {
    if (logLevel < FLAGS_minloglevel) {
        return;
    }
    auto ctx = std::make_shared<LoggingContext>();
    auto item = ctx->request.add_items();
    auto msg = item->mutable_msg();

    FormatHeader(*msg, file, line, logLevel);
    StringUtils::VFormat(*msg, fmt, args);
    msg->append(1, '\n');

    //item->set_pid(ProcessUtils::get_pid());
    //item->set_tid(ProcessUtils::get_tid());
    item->set_file(file);
    item->set_line(line);
    item->set_serverity(logLevel == LOG_LEVEL_FATAL ? LOG_LEVEL_ERROR : logLevel);
    {
        std::lock_guard<std::mutex> lock(lock_);
        log_queue_.push(ctx);
    }
    NotifyLog();

    if (logLevel == LOG_LEVEL_FATAL) {
        g_RuntimeLogger->Fatal(file, line, msg->c_str());
    }
}

void Logger::Log(const char *file, int line, int logLevel, const char* data, size_t len) {
    if (logLevel < FLAGS_minloglevel) {
        return;
    }
    auto ctx = std::make_shared<LoggingContext>();
    auto item = ctx->request.add_items();
    auto msg = item->mutable_msg();

    FormatHeader(*msg, file, line, logLevel);
    msg->append(data, len);
    msg->append(1, '\n');

    //item->set_pid(ProcessUtils::get_pid());
    //item->set_tid(ProcessUtils::get_tid());
    item->set_file(file);
    item->set_line(line);
    item->set_serverity(logLevel == LOG_LEVEL_FATAL ? LOG_LEVEL_ERROR : logLevel);
    {
        std::lock_guard<std::mutex> lock(lock_);
        log_queue_.push(ctx);
    }
    NotifyLog();

    if (logLevel == LOG_LEVEL_FATAL) {
        g_RuntimeLogger->Fatal(file, line, msg->c_str());
    }
}

void Logger::Log(const char* data, size_t len) {
    auto ctx = std::make_shared<LoggingContext>();
    auto item = ctx->request.add_items();
    auto msg = item->mutable_msg();
    msg->append(data, len);
    msg->append(1, '\n');
    item->set_serverity(LOG_LEVEL_INFO);
    {
        std::lock_guard<std::mutex> lock(lock_);
        log_queue_.emplace(ctx);
    }
    NotifyLog();
}

void Logger::Info(const char* file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogFormat(file, line, LOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

void Logger::Warning(const char* file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogFormat(file, line, LOG_LEVEL_WARN, fmt, args);
    va_end(args);
}
void Logger::Error(const char* file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogFormat(file, line, LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}
void Logger::Fatal(const char* file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogFormat(file, line, LOG_LEVEL_FATAL, fmt, args);
    va_end(args);
}

void Logger::Flush() {
    auto ctx = std::make_shared<LoggingContext>();
    ctx->request.set_flush_level(LOG_LEVEL_DEBUG);
    {
        std::lock_guard<std::mutex> lock(lock_);
        log_queue_.emplace(std::move(ctx));
    }
    NotifyLog();
}

void Logger::WriteFileLog(const char *basename, const char* data, size_t len) {
    auto ctx = std::make_shared<LoggingContext>();
    ctx->request.set_basename(basename);
    auto item = ctx->request.add_items();
    auto msg = item->mutable_msg();

    msg->append(data, len);

    item->set_serverity(LOG_LEVEL_INFO);
    ctx->request.set_flush_level(LOG_LEVEL_INFO);
    {
        std::lock_guard<std::mutex> lock(lock_);
        log_queue_.push(ctx);
    }
    NotifyLog();
}

void Logger::FlushFileLog(const char* basename) {
    auto ctx = std::make_shared<LoggingContext>();
    ctx->request.set_basename(basename);
    ctx->request.set_flush_level(LOG_LEVEL_INFO);
    {
        std::lock_guard<std::mutex> lock(lock_);
        log_queue_.emplace(std::move(ctx));
    }
    NotifyLog();
}

void Logger::RunActions() {
    std::queue<LoggingContextPtr> logs;
    {
        std::lock_guard<std::mutex> lock(lock_);
        log_queue_.swap(logs);
    }
    while (logs.size() > 0) {
        auto ctx = logs.front();
        logs.pop();
        if (server_mode_) { //just log locally
            service_->Log(ctx->request);
        } else {
            client_->Log(ctx);
        }
    }
}

void Logger::LogToFile(const char* file, int line, int logLevel, const std::string& msg) {
    if (server_mode_ && destination_) {
        destination_->LogToFile(logLevel, &msg[0], msg.length());
    }
}

void Logger::LogToFile(const std::string& basename, const char* file, int line, int logLevel, const std::string& msg) {
    if (server_mode_ && destination_) {
        destination_->LogToFile(basename, &msg[0], msg.length());
    }
}

void Logger::FlushLogFile(int level) {
    if (server_mode_ && destination_) {
        destination_->Flush(level);
    }
}

void Logger::FlushLogFile(const std::string& base_filename) {
    if (server_mode_ && destination_) {
        destination_->Flush(base_filename);
    }
}

void Logger::FormatHeader(std::string& msg, const char* file, int line, int logLevel) {
    logLevel = (logLevel >= LOG_LEVEL_DEBUG && logLevel <= LOG_LEVEL_FATAL) ? logLevel : LOG_LEVEL_INFO;
    const char* date_string = DateUtils::NowDateString();
    int tid = ProcessUtils::get_tid();
    msg.append(1, '[')
    .append(date_string)
    .append(1, ' ')
    .append(host_)
    .append(1, ' ')
    .append(pid_)
    .append(1, ':')
    .append(std::to_string(tid))
    .append(1, ' ')
    .append(file)
    .append(1, ':')
    .append(std::to_string(line))
    .append(1, ' ')
    .append(LOG_LEVEL_NAMES[logLevel])
    .append(1, ']')
    .append(1, ' ');
    /*StringUtils::Format(msg, "[%s %s %d:%d %s:%d %s] ", date_string,
                        host_.c_str(), pid_, tid, name, line, LOG_LEVEL_NAMES[logLevel]);*/
}

bool Logger::IsServerMode() {
    //Unix socket
    if (StringUtils::EndsWith(path_, ".sock")) {
        return false;
    }
    return true;
}

void Logger::NotifyLog() {
    event_loop_->Signal(kLogSignal);
}

void Logger::NotifyExit() {
    event_loop_->Signal(kExitSingal);
}

#define GLOG_OPTION_IMPL(OPT, TYPE) \
void Logger::set_##OPT(TYPE value) {\
	FLAGS_##OPT = value;\
    auto ctx = std::make_shared<LoggingContext>();;\
	auto opts = ctx->request.mutable_options();\
	opts->set_##OPT(value);\
	{\
		std::lock_guard<std::mutex> lock(lock_);\
		log_queue_.emplace(std::move(ctx));\
	}\
	NotifyLog();\
}

GLOG_OPTION_IMPL(logtostderr, bool)
GLOG_OPTION_IMPL(logbufsecs, int)
GLOG_OPTION_IMPL(minloglevel, int)
GLOG_OPTION_IMPL(max_log_size, int)
}
}
