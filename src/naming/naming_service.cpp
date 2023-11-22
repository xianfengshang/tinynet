// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "naming_service.h"
#include "naming_state.h"
#include "raft/raft_service.h"
#include "raft/raft_types.h"
#include "rpc/rpc_server.h"
#include "naming_rpc_service_impl.h"
#include "util/string_utils.h"
#include "base/error_code.h"

namespace tinynet {
namespace naming {
NamingService::NamingService(EventLoop *loop) :
    event_loop_(loop) {
    (void)event_loop_;
}

int NamingService::Init(raft::RaftService* raft_service, const raft::NodeConfig& raft_node) {
    int err = ERROR_OK;
    state_.reset(new (std::nothrow) NamingState(raft_service));
    if (!state_) {
        err = ERROR_OS_OOM;
        return err;
    }
    err = state_->Init(raft_node);
    return err;
}

void NamingService::RegisterService(rpc::RpcServer *server) {
    auto service_impl = std::make_shared<NamingRpcServiceImpl>(this);
    server->RegisterService(std::static_pointer_cast<google::protobuf::Service>(service_impl));
}

}
}
