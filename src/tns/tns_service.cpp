// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tns_service.h"
#include "raft/raft_service.h"
#include "raft/raft_types.h"
#include "rpc/rpc_server.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"
#include "util/net_utils.h"
#include "naming/naming_service.h"
#include "logging/logging.h"
#include "base/error_code.h"

namespace tinynet {
namespace tns {
TnsService::TnsService(EventLoop *loop) :
    event_loop_(loop),
    port_(0) {
}

int TnsService::Init(const std::string& name, const raft::NodeConfig& raft_config) {
    name_ = name;
    int err = 0;
    server_.reset(new(std::nothrow) tinynet::rpc::RpcServer(event_loop_));
    if (!server_) {
        err = ERROR_OS_OOM;
        return err;
    }
    raft_.reset(new(std::nothrow) raft::RaftService(event_loop_));
    if (!server_) {
        err = ERROR_OS_OOM;
        return err;
    }
    raft_->RegisterService(server_.get());

    naming_.reset(new(std::nothrow) naming::NamingService(event_loop_));
    if (!naming_) {
        err = ERROR_OS_OOM;
        return err;
    }
    if ((err = naming_->Init(raft_.get(), raft_config))) {
        log_error("Init naming service failed");
        return err;
    }
    naming_->RegisterService(server_.get());
    return err;
}

int TnsService::Start(const std::string& addr) {
    std::string host;
    int err = ERROR_OK;
    if (!UriUtils::parse_address(addr, &host, &port_)) {
        err = ERROR_URI_UNRECOGNIZED;
        return err;
    }
    if (!NetUtils::GetLocalIP(&host)) {
        err = ERROR_OS_ADAPTERINFO;
        return err;
    }
    if (!server_) {
        err = ERROR_INVAL;
        return err;
    }
    if ((err = server_->Start("0.0.0.0", port_))) {
        return err;
    }
    log_info("TNS service started at %s successfully", addr.c_str());
    return err;
}

void TnsService::Stop() {
    if (server_) {
        server_->Stop();
    }
    if (raft_) {
        raft_->Stop();
    }
}
}
}