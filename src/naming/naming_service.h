// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "naming_state.h"

namespace tinynet {
class EventLoop;
namespace rpc {
class RpcServer;
}
namespace raft {
class RaftService;
struct NodeConfig;
}
namespace naming {

class NamingService {
  public:
    NamingService(EventLoop *loop);
  public:
    int Init(raft::RaftService* raft_service, const raft::NodeConfig& raft_node);

    void RegisterService(rpc::RpcServer *server);
  public:
    NamingState* get_state() { return state_.get(); }
  private:
    EventLoop * event_loop_;
    std::unique_ptr<NamingState> state_;
};
}
}
