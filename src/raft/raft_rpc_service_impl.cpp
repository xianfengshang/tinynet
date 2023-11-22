// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_rpc_service_impl.h"
#include "raft/raft_service.h"
#include "raft/raft_node.h"
namespace tinynet {
namespace raft {

RaftRpcServiceImpl::RaftRpcServiceImpl(RaftService * service) :
    service_(service) {

}

void RaftRpcServiceImpl::RequestVote(::google::protobuf::RpcController* controller, const ::tinynet::raft::VoteReq* request, ::tinynet::raft::VoteResp* response, ::google::protobuf::Closure* done) {
    auto node = service_->GetNode(request->peerid());
    node->RequestVote(controller, request, response, done);
}

void RaftRpcServiceImpl::AppendEntries(::google::protobuf::RpcController* controller, const ::tinynet::raft::AppendEntriesReq* request, ::tinynet::raft::AppendEntriesResp* response, ::google::protobuf::Closure* done) {
    auto node = service_->GetNode(request->peerid());
    node->AppendEntries(controller, request, response, done);
}

void RaftRpcServiceImpl::InstallSnapshot(::google::protobuf::RpcController* controller, const ::tinynet::raft::InstallSnapshotReq* request, ::tinynet::raft::InstallSnapshotResp* response, ::google::protobuf::Closure* done) {
    auto node = service_->GetNode(request->peerid());
    node->InstallSnapshot(controller, request, response, done);
}
}
}
