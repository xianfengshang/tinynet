// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/event_loop.h"
#include "raft_types.h"
#include <vector>
#include <memory>
#include <unordered_map>
namespace tinynet {
namespace rpc {
class RpcServer;
}
namespace raft {
class RaftService;
typedef std::shared_ptr<RaftService> RaftServicePtr;

class RaftChannel;
using RaftChannelPtr = std::shared_ptr<RaftChannel>;
class RaftNode;
using RaftNodePtr = std::shared_ptr<RaftNode>;

class RaftStateMachine;

class RaftService {
  public:
    RaftService(EventLoop *loop);
    ~RaftService();
  public:
    void Stop();

    RaftNodePtr CreateNode(const NodeConfig &config, RaftStateMachine * state_machine, int* err);

    RaftNodePtr GetNode(int nodeId);

    void RemoveNode(int nodeId);

    void RegisterService(rpc::RpcServer *server);
  public:
    EventLoop * event_loop() { return event_loop_; }
  private:
    using RaftNodeMap = std::unordered_map<int, RaftNodePtr>;
    EventLoop* event_loop_;
    RaftNodeMap nodes_;
};
}
}
