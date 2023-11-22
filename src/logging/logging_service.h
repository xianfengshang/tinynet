// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <unordered_map>
#include <memory>

namespace tinynet {
class EventLoop;
namespace rpc {
class RpcServer;
}
namespace logging {
class LoggingService;
typedef std::shared_ptr<LoggingService> LoggingServicePtr;
class LogReq;

class LoggingService {
  public:
    LoggingService(EventLoop *loop);
  public:
    void RegisterService(rpc::RpcServer *server);
  public:
    void Log(const LogReq& req);
  private:
    EventLoop * event_loop_;
};
}
}
