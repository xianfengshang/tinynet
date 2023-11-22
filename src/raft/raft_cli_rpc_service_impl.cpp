// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_cli_rpc_service_impl.h"
namespace tinynet {
namespace raft {
RaftCliRpcServiceImpl::RaftCliRpcServiceImpl(RaftService *service) :
    service_(service) {
    (void)(service_);

}

void RaftCliRpcServiceImpl::GetLeader(::google::protobuf::RpcController* controller, const ::tinynet::raft::GetLeaderReq* request, ::tinynet::raft::GetLeaderResp* response, ::google::protobuf::Closure* done) {
    //TODO
}
}
}
