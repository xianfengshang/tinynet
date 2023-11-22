// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "raft.pb.h"
namespace tinynet {
namespace raft {
class RaftService;

class RaftRpcServiceImpl :
    public RaftRpcService {
  public:
    explicit RaftRpcServiceImpl(RaftService * service);
  public:

    virtual void RequestVote(::google::protobuf::RpcController* controller,
                             const ::tinynet::raft::VoteReq* request,
                             ::tinynet::raft::VoteResp* response,
                             ::google::protobuf::Closure* done) override;

    virtual void AppendEntries(::google::protobuf::RpcController* controller,
                               const ::tinynet::raft::AppendEntriesReq* request,
                               ::tinynet::raft::AppendEntriesResp* response,
                               ::google::protobuf::Closure* done) override;
    virtual void InstallSnapshot(::google::protobuf::RpcController* controller,
                                 const ::tinynet::raft::InstallSnapshotReq* request,
                                 ::tinynet::raft::InstallSnapshotResp* response,
                                 ::google::protobuf::Closure* done) override;
  private:
    RaftService * service_;
};
}
}
