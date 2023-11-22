// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include <algorithm>
#include "cluster_service.h"
#include "util/string_utils.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "raft/raft_types.h"
#include "naming/naming_service.h"
#include "raft/raft_service.h"
namespace tinynet {
namespace cluster {

static raft::NodeConfig* BuildRaftConfig(const std::string& id, const ClusterOptions& opts, raft::NodeConfig *config) {
    config->id = -1;
    for (size_t i = 0; i < opts.namingService.servers.size(); ++i) {
        auto& info = opts.namingService.servers[i];
        config->peers.push_back(info.url);
        if (id == info.id) {
            config->id = static_cast<int>(i);
        }
    }
    if (config->id == -1) return nullptr;

    config->debugMode = opts.namingService.debugMode;
    config->standalong = opts.namingService.servers.size() == 1;
    if (!opts.namingService.dataDir.empty()) {
        config->dataDir.append(opts.namingService.dataDir);
        if (!StringUtils::EndWith(opts.namingService.dataDir, "/\\")) {
            config->dataDir.append("/");
        }
    }
    config->dataDir.append(id);
    config->snapshotCount = opts.namingService.snapshotCount;
    config->heartbeatInterval = opts.namingService.heartbeatInterval;
    config->electionTimeout = opts.namingService.electionTimeout;
    return config;
}

ClusterService::ClusterService(tinynet::EventLoop* loop ) :
    event_loop_(loop) {
}

void ClusterService::Init() {
}

int ClusterService::Start(const std::string& id, const ClusterOptions& opts) {
    int err = StartNaming(id, opts);
    if (err == ERROR_OK || err != ERROR_RAFT_CONFIGURATION) {
        return err;
    }
    err = StartCluster(id, opts);
    return err;
}


int ClusterService::StartNaming(const std::string& id, const ClusterOptions& opts) {
    int err = ERROR_OK;
    if (tns_map_.find(id) != tns_map_.end()) {
        return err;
    }
    raft::NodeConfig node_config;
    if (!BuildRaftConfig(id, opts, &node_config)) {
        err = ERROR_RAFT_CONFIGURATION;
        return err;
    }
    auto tns = std::make_shared<tns::TnsService>(event_loop_);
    if (!tns) {
        err = ERROR_OS_OOM;
        return err;
    }
    if ((err = tns->Init(id, node_config))) {
        return err;
    }
    node_config.peers[node_config.id];
    if ((err = tns->Start(node_config.peers[node_config.id]))) {
        return err;
    }
    tns_map_[id] = tns;
    log_info("Naming node started successfully!");
    return err;
}

int ClusterService::StartCluster(const std::string& id, const ClusterOptions& opts) {
    int err = ERROR_OK;
    if (tdc_map_.find(id) != tdc_map_.end()) {
        return err;
    }
    auto tdc = std::make_shared<tdc::TdcService>(event_loop_);
    if (!tdc) {
        err = ERROR_OS_OOM;
        return err;
    }

    tdc::TdcOptions tdc_opts;
    for (auto& info : opts.namingService.servers) {
        tdc_opts.tns_addrs.push_back(info.url);
    }

    tdc_opts.name = id;
    tdc_opts.nameSpace = opts.namingService.nameSpace;
    tdc_opts.debugMode = opts.namingService.debugMode;
    tdc_opts.registrationInterval = opts.namingService.registrationInterval;
    tdc_opts.expiryTime = opts.namingService.expiryTime;
    tdc->Init(tdc_opts);
    if ((err = tdc->Start(opts.servicePortRange))) {
        return err;
    }
    tdc_map_[id] = tdc;
    log_info("Cluster node started successfully!");
    return err;
}

void ClusterService::Stop() {
    for (auto& entry : tns_map_) {
        entry.second->Stop();
    }
    for (auto& entry : tdc_map_) {
        entry.second->Stop();
    }
}

void ClusterService::SendMsg(const std::string& name, const std::string &body, tdc::TdcMessageCallback callback) {
    if (tdc_map_.empty()) return;

    for (auto& entry : tdc_map_) {
        if (entry.second->ExistsChannel(name)) {
            entry.second->SendMsg(name, body, std::move(callback));
            return;
        }
    }
    tdc_map_.begin()->second->SendMsg(name, body, std::move(callback));
}

void ClusterService::SendMsg(const std::string& name, const void* body, size_t len, tdc::TdcMessageCallback callback) {
    if (tdc_map_.empty()) return;

    for (auto& entry : tdc_map_) {
        if (entry.second->ExistsChannel(name)) {
            entry.second->SendMsg(name, body, len, std::move(callback));
            return;
        }
    }
    tdc_map_.begin()->second->SendMsg(name, body, len, std::move(callback));
}

void ClusterService::SendMsg(const std::string& name, tdc::TdcMessagePtr msg) {
    if (tdc_map_.empty()) return;

    for (auto& entry : tdc_map_) {
        if (entry.second->ExistsChannel(name)) {
            entry.second->SendMsg(name, std::move(msg));
            return;
        }
    }
    tdc_map_.begin()->second->SendMsg(name, std::move(msg));
}

std::shared_ptr<tdc::TdcService> ClusterService::get_tdc(const std::string& id) {
    auto it = tdc_map_.find(id);
    if (it == tdc_map_.end()) {
        return nullptr;
    }
    return it->second;
}

int ClusterService::Put(const std::string &name, const std::string &value, uint32_t timeout, naming::NamingResolver::NamingCallback callback) {
    if (tdc_map_.empty()) return ERROR_TDC_NOSTUB;

    return tdc_map_.begin()->second->get_resolver()->Put(name, value, timeout, std::move(callback));
}

int ClusterService::Get(const std::string &name, naming::NamingResolver::NamingCallback callback) {
    if (tdc_map_.empty()) return ERROR_TDC_NOSTUB;

    return tdc_map_.begin()->second->get_resolver()->Get(name, std::move(callback));
}

int ClusterService::Delete(const std::string &name, naming::NamingResolver::NamingCallback callback) {
    if (tdc_map_.empty()) return ERROR_TDC_NOSTUB;

    return tdc_map_.begin()->second->get_resolver()->Delete(name, std::move(callback));
}

int ClusterService::Keys(const std::string& name, naming::NamingResolver::NamingCallback callback) {
    if (tdc_map_.empty()) return ERROR_TDC_NOSTUB;
    return tdc_map_.begin()->second->get_resolver()->Keys(name, std::move(callback));
}

}
}
