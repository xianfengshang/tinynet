// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "raft_service.h"
#include "raft_rpc_service_impl.h"
#include "rpc/rpc_server.h"
#include "raft_node.h"
#include "logging/logging.h"
#include "util/string_utils.h"
#include "rpc/rpc_server.h"
#include "base/error_code.h"

namespace tinynet {
namespace raft {
RaftService::RaftService(EventLoop *loop ) :
    event_loop_(loop) {
}

RaftService::~RaftService() = default;


void RaftService::Stop() {
    for (auto node : nodes_) {
        node.second->Stop();
    }
}

RaftNodePtr RaftService::CreateNode(const NodeConfig &config, RaftStateMachine *state_machine, int* err) {
    auto node = std::make_shared<RaftNode>(this, state_machine);
    if (!node) {
        *err = ERROR_OS_OOM;
        return RaftNodePtr();
    }
    if ((*err = node->Init(config))) {
        return RaftNodePtr();
    }
    nodes_[node->get_id()] = node;
    *err = ERROR_OK;
    return node;
}

RaftNodePtr RaftService::GetNode(int nodeId) {
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return nullptr;
    }
    return it->second;
}

void RaftService::RemoveNode(int nodeId) {
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        nodes_.erase(it);
    }
}

void RaftService::RegisterService(rpc::RpcServer *server) {
    auto service_impl = std::make_shared<RaftRpcServiceImpl>(this);
    server->RegisterService(std::static_pointer_cast<google::protobuf::Service>(service_impl));
}
}
}
