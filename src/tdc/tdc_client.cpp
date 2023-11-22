// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tdc_client.h"
#include "tdc_service.h"
#include "base/error_code.h"
namespace tinynet {
namespace tdc {
TdcClient::TdcClient(TdcService *service) :
    service_(service) {

}

TdcChannelPtr TdcClient::GetChannel(const std::string& name) {
    auto it = channels_.find(name);
    if (it != channels_.end()) {
        return it->second;
    }
    auto channel = std::make_shared<TdcChannel>(name, service_);
    channel->Init();
    channels_[channel->get_name()] = channel;
    return channel;
}

TdcChannelPtr TdcClient::GetChannel(int64_t guid) {
    for (auto &item : channels_) {
        auto &channel = item.second;
        if (channel->get_guid() == guid) {
            return channel;
        }
    }
    return nullptr;
}

TdcChannelPtr TdcClient::RemoveChannel(const std::string& name) {
    auto it = channels_.find(name);
    if (it == channels_.end()) {
        return nullptr;
    }
    auto channel = it->second;
    channels_.erase(it);
    return channel;
}

void TdcClient::Stop() {
    if (channels_.size() == 0) {
        return;
    }
    for (auto channel : channels_) {
        channel.second->Run(ERROR_RPC_REQUESTCANCELED);
    }
    ChannelMap empty;
    channels_.swap(empty);
}

}
}
