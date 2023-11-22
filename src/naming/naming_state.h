// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <unordered_map>
#include "rpc/rpc_info.h"
#include "raft/raft_types.h"
#include "raft/raft_state_machine.h"
#include "kv_db.h"
#include "naming.pb.h"

namespace tinynet {

namespace raft {
class RaftService;
class RaftNode;
using RaftNodePtr = std::shared_ptr<RaftNode> ;
}

namespace naming {

class NamingState :
    public raft::RaftStateMachine {
  public:
    NamingState(raft::RaftService* raft_service);
  public:
    int Init(const raft::NodeConfig& raft_config);
    // @RaftStateMachine
    void StateChanged(raft::StateType role) override;
    void SaveSnapshot(IOBuffer* buffer) override;
    void LoadSnapshot(const char* data, size_t len) override;
    void ApplyEntry(uint64_t logIndex, const std::string& data);
  public:
    void Put(::google::protobuf::RpcController* controller,
             const ::tinynet::naming::ClientRequest* request,
             ::tinynet::naming::ClientResponse* response,
             ::google::protobuf::Closure* done);
    void Get(::google::protobuf::RpcController* controller,
             const ::tinynet::naming::ClientRequest* request,
             ::tinynet::naming::ClientResponse* response,
             ::google::protobuf::Closure* done);
    void Delete(::google::protobuf::RpcController* controller,
                const ::tinynet::naming::ClientRequest* request,
                ::tinynet::naming::ClientResponse* response,
                ::google::protobuf::Closure* done);
    void Keys(::google::protobuf::RpcController* controller,
              const ::tinynet::naming::ClientRequest* request,
              ::tinynet::naming::ClientResponse* response,
              ::google::protobuf::Closure* done);
  private:

    void HandleApplyPut(uint64_t logIndex,
                        const ::tinynet::naming::ClusterMessage msg);
    void HandleApplyDel(uint64_t logIndex,
                        const ::tinynet::naming::ClusterMessage msg);
  public:
    const std::string& get_name() const { return name_; }
  private:
    rpc::RpcInfoPtr PopCall(uint64_t logIndex);
    void SendResponse(rpc::RpcInfoPtr& call, int error_code);
    void SendRedirect(rpc::RpcInfoPtr& call);
  public:
    using CallMap = std::unordered_map<int64_t, rpc::RpcInfoPtr>;
  private:
    std::string			name_;
    raft::RaftService*  raft_;
    raft::RaftNodePtr	node_;
    KVDB				db_;
    CallMap				calls_;
};
}
}
