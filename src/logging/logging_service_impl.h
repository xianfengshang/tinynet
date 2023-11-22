// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "logging.pb.h"
#include <memory>
namespace tinynet {
namespace logging {
class LoggingServiceImpl;
typedef std::shared_ptr<LoggingServiceImpl> LoggingServiceImplPtr;
class LoggingService;
class LoggingServiceImpl :
    public LoggingRpcService {
  public:
    LoggingServiceImpl(LoggingService *service);
  public:
    virtual void Log(::google::protobuf::RpcController* controller,
                     const ::tinynet::logging::LogReq* request,
                     ::tinynet::logging::LogResp* response,
                     ::google::protobuf::Closure* done) override;
  private:
    LoggingService * service_;
};
}
}
