// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "rpc_method.h"
namespace tinynet {
namespace rpc {
RpcMethod::RpcMethod(std::shared_ptr<google::protobuf::Service> service, const google::protobuf::MethodDescriptor* descriptor) :
    service_(std::move(service)),
    descriptor_(descriptor) {

}
google::protobuf::Message* RpcMethod::NewRequest() {
    return service_->GetRequestPrototype(descriptor_).New();
}
google::protobuf::Message* RpcMethod::NewResponse() {
    return service_->GetResponsePrototype(descriptor_).New();
}
}
}
