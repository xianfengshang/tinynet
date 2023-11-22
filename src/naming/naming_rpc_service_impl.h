// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "naming.pb.h"
namespace tinynet {
namespace naming {
class NamingService;

class NamingRpcServiceImpl :
    public NamingRpcService {
  public:
    NamingRpcServiceImpl(NamingService *service);
  public:
    virtual void Invoke(::google::protobuf::RpcController* controller,
                        const ::tinynet::naming::ClientRequest* request,
                        ::tinynet::naming::ClientResponse* response,
                        ::google::protobuf::Closure* done);
  private:
    NamingService * service_;
};
}
}
