// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "cli.pb.h"
namespace tinynet {
namespace raft {
class RaftService;
class RaftCliRpcServiceImpl :
    public RaftCliRpcService {
  public:
    explicit RaftCliRpcServiceImpl(RaftService *service);
  public:
    virtual void GetLeader(::google::protobuf::RpcController* controller,
                           const ::tinynet::raft::GetLeaderReq* request,
                           ::tinynet::raft::GetLeaderResp* response,
                           ::google::protobuf::Closure* done
                          ) override;
  private:
    RaftService * service_;
};
}
}
