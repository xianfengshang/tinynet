// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <functional>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include "net/event_loop.h"
#include "rpc/rpc_channel.h"
#include "rpc/rpc_controller.h"
#include "logging.pb.h"

namespace tinynet {
namespace logging {

typedef std::function<void(int err)> LoggingCallback;

struct LoggingContext {
    logging::LogReq request;
    logging::LogResp response;
    rpc::RpcController controller;
    LoggingCallback callback;
};
typedef std::shared_ptr<LoggingContext> LoggingContextPtr;

class LoggerClient {
  public:
    LoggerClient(EventLoop *loop);
    ~LoggerClient();
  public:
    void Init(const std::string& name);
    void Stop();
  public:
    typedef std::function<void(int err)> LoggerCallback;

    void Log(LoggingContextPtr ctx);

    rpc::RpcChannel* CreateChannel(const std::string& name);

    void AfterLog(LoggingContextPtr ctx);

    using StubPtr = std::unique_ptr<LoggingRpcService_Stub>;
  private:
    EventLoop * event_loop_;
    std::string name_;
    std::unique_ptr<rpc::RpcChannel> channel_;
    StubPtr stub_;
};
}
}
