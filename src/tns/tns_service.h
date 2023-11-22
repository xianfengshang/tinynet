// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <memory>
#include "rpc/rpc_server.h"
#include "naming/naming_service.h"
#include "raft/raft_service.h"

namespace tinynet {
namespace tns {

class TnsService {
  public:
    TnsService(EventLoop *loop);
  public:
    //Initialize tns service
    int Init(const std::string& name, const raft::NodeConfig& raft_config);
    //Start tns service with a given address
    int Start(const std::string& addr);
    //Stop tns service
    void Stop();
  public:
  private:
    std::string name_;
    EventLoop* event_loop_;
    std::unique_ptr<rpc::RpcServer>	server_;
    std::unique_ptr<raft::RaftService> raft_;
    std::unique_ptr<naming::NamingService> naming_;
    int port_;
};
}
}
