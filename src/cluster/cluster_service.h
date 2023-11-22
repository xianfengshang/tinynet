// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/event_loop.h"
#include "cluster_types.h"
#include "tdc/tdc_service.h"
#include "tns/tns_service.h"
#include <map>

namespace tinynet {
namespace cluster {

class ClusterService  {
  public:
    ClusterService(tinynet::EventLoop* loop);
  public:
    void Init();
    int Start(const std::string& id, const ClusterOptions& options);
    void Stop();
  private:
    //Startup name node
    int StartNaming(const std::string& id, const ClusterOptions& opts);
    //Startup cluster node
    int StartCluster(const std::string& id, const ClusterOptions& opts);
  public:
    void SendMsg(const std::string& name, const std::string &body, tdc::TdcMessageCallback callback);
    void SendMsg(const std::string& name, const void* body, size_t len, tdc::TdcMessageCallback callback);
    void SendMsg(const std::string& name, tdc::TdcMessagePtr msg);

    int Put(const std::string &name, const std::string &value, uint32_t timeout, naming::NamingResolver::NamingCallback callback);

    int Get(const std::string &name, naming::NamingResolver::NamingCallback callback);

    int Delete(const std::string &name, naming::NamingResolver::NamingCallback callback);

    int Keys(const std::string& name, naming::NamingResolver::NamingCallback callback);
  public:
    const std::map<std::string, std::shared_ptr<tdc::TdcService>>& tdc_map() { return tdc_map_; }
    size_t tdc_size() const { return tdc_map_.size(); }
    std::shared_ptr<tdc::TdcService> get_tdc(const std::string& id);
  private:
    tinynet::EventLoop* event_loop_;
    std::map<std::string, std::shared_ptr<tns::TnsService>> tns_map_;
    std::map<std::string, std::shared_ptr<tdc::TdcService>> tdc_map_;
};
}
}
