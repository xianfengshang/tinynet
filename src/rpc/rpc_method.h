// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "google/protobuf/service.h"
#include "google/protobuf/message.h"
#include <memory>
namespace tinynet {
namespace rpc {
class RpcMethod;
typedef std::shared_ptr<RpcMethod> RpcMethodPtr;
class RpcMethod {
  public:
    RpcMethod(std::shared_ptr<google::protobuf::Service> service,
              const google::protobuf::MethodDescriptor* descriptor);
  public:
    std::shared_ptr<google::protobuf::Service> get_service() {
        return service_;
    }
    const google::protobuf::MethodDescriptor* get_descriptor() {
        return descriptor_;
    }
  public:
    google::protobuf::Message* NewRequest();
    google::protobuf::Message* NewResponse();
  private:
    std::shared_ptr<google::protobuf::Service> service_;
    const google::protobuf::MethodDescriptor* descriptor_;
};
}
}
