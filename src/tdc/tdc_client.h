// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "net/event_loop.h"
#include <memory>
#include <unordered_map>
#include "tdc_channel.h"
#include "naming/naming_resolver.h"
namespace tinynet {
namespace tdc {
class TdcClient;
typedef std::shared_ptr<TdcClient> TdcClientPtr;

class TdcService;

// Manage distributed communication channels
class TdcClient {
  public:
    //Constructor
    TdcClient(TdcService *service);
  public:
    //Get a channel by channel name. Create a new one if no such channel exists.
    TdcChannelPtr GetChannel(const std::string &name);
    //Get a channel by guid.
    TdcChannelPtr GetChannel(int64_t guid);
    //Remove the channel by channel name
    TdcChannelPtr RemoveChannel(const std::string &name);
    void Stop();
  private:
    using ChannelMap = std::unordered_map<std::string, TdcChannelPtr>;
    TdcService *service_;
    ChannelMap channels_;
};
}
}
