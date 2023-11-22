// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "logging_service.h"
#include "raft/raft_service.h"
#include "raft/raft_types.h"
#include "rpc/rpc_server.h"
#include "logging_service_impl.h"
#include "util/string_utils.h"
#include "logging.h"
#include "logger.h"

namespace tinynet {
namespace logging {
LoggingService::LoggingService(EventLoop *loop) :
    event_loop_(loop) {
    (void)event_loop_;
}

void LoggingService::RegisterService(rpc::RpcServer *server) {
    auto service_impl = std::make_shared<tinynet::logging::LoggingServiceImpl>(this);
    server->RegisterService(std::static_pointer_cast<google::protobuf::Service>(service_impl));
}

void LoggingService::Log(const LogReq& req) {
    // + set options
    if (req.has_options()) {
        auto& options = req.options();
        if (options.has_logtostderr()) {
            FLAGS_logtostderr = options.logtostderr();
        }
        if (options.has_logbufsecs()) {
            FLAGS_logbufsecs = options.logbufsecs();
        }
        if (options.has_minloglevel()) {
            FLAGS_minloglevel = options.minloglevel();
        }
        if (options.has_max_log_size()) {
            FLAGS_max_log_size = options.max_log_size();
        }
    }
    // + log message
    for (int i = 0; i < req.items_size(); ++i) {
        auto& item = req.items(i);
        if (req.has_basename()) {
            g_Logger->LogToFile(req.basename(), item.file().c_str(), item.line(), item.serverity(), item.msg());
        } else {
            g_Logger->LogToFile(item.file().c_str(), item.line(), item.serverity(), item.msg());
        }
    }
    // + flush log file
    if (req.has_flush_level()) {
        if (req.has_basename()) {
            g_Logger->FlushLogFile(req.basename());
        } else {
            g_Logger->FlushLogFile(req.flush_level());
        }
    }
}
}
}
