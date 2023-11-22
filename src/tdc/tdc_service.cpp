// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tdc_service.h"
#include "tdc_rpc_service_impl.h"
#include "util/net_utils.h"
#include "logging/logging.h"
#include "base/error_code.h"
#include "util/string_utils.h"
#include "util/uri_utils.h"

namespace tinynet {
namespace tdc {


static const int kNameRegistrationInterval = 30 * 1000;

static const int kNameExpiryTime = kNameRegistrationInterval * 2;

static const uint64_t kNameRegistrationFailedWarningThreshold = 30;

static const uint64_t kNameRegistrationSuccessloggingThreshold = 360;

TdcService::TdcService(EventLoop *loop) :
    event_loop_(loop),
    register_timer_(INVALID_TIMER_ID),
    receive_msg_cb_(nullptr),
    failed_count_(0),
    success_count_(0) {
}

TdcService::~TdcService() {
    if (register_timer_) {
        event_loop_->ClearTimer(register_timer_);
    }
}

void TdcService::Init(const TdcOptions& opts) {
    options_ = opts;
    options_.registrationInterval = opts.registrationInterval <= 0 ? kNameRegistrationInterval : opts.registrationInterval;
    options_.expiryTime = opts.expiryTime <= 0 ? kNameExpiryTime : opts.expiryTime;
    root_dir_.append(options_.nameSpace);
    if (!StringUtils::EndWith(root_dir_, ":/")) {
        root_dir_.append("/");
    }
    address_.first.append(root_dir_).append(options_.name);
    resolver_.reset(new(std::nothrow)naming::NamingResolver(event_loop_));
    resolver_->Init(opts.tns_addrs);
    server_.reset(new(std::nothrow) rpc::RpcServer(event_loop_));
    auto service_impl = std::make_shared<TdcRpcServiceImpl>(this);
    server_->RegisterService(std::static_pointer_cast<::google::protobuf::Service>(service_impl));
}

int TdcService::Start(const std::tuple<int, int>& ports)  {
    int err = ERROR_OK;
    if ((err = server_->Start(ports))) {
        return err;
    }
    std::string host;
    if (!NetUtils::GetLocalIP(&host)) {
        err = ERROR_OS_ADAPTERINFO;
        return err;
    }
    StringUtils::Format(address_.second, "tcp://%s:%d", host.c_str(), server_->get_listen_port());
    register_timer_ = event_loop_->AddTimer(0, options_.registrationInterval, std::bind(&TdcService::RegisterService, this));
    return err;
}

int TdcService::Start(const std::string& addr) {
    std::string host;
    int port = 0;
    int err = ERROR_OK;
    if (!UriUtils::parse_address(addr, &host, &port)) {
        err = ERROR_URI_UNRECOGNIZED;
        return err;
    }
    if (!NetUtils::GetLocalIP(&host)) {
        err = ERROR_OS_ADAPTERINFO;
        return err;
    }
    if ((err = server_->Start("0.0.0.0", port)) != ERROR_OK) {
        return err;
    }
    UriUtils::format_address(address_.second, "tcp", host, &port);
    register_timer_ = event_loop_->AddTimer(0, options_.registrationInterval, std::bind(&TdcService::RegisterService, this));
    return err;
}

bool TdcService::ExistsChannel(const std::string& name) {
    return channels_.find(name) != channels_.end();
}

TdcChannelPtr TdcService::GetChannel(const std::string& name) {
    auto it = channels_.find(name);
    if (it != channels_.end()) {
        return it->second;
    }
    auto channel = std::make_shared<TdcChannel>(name, this);
    channel->Init();
    channels_[channel->get_name()] = channel;
    return channel;
}

TdcChannelPtr TdcService::GetChannel(int64_t guid) {
    for (auto &item : channels_) {
        auto &channel = item.second;
        if (channel->get_guid() == guid) {
            return channel;
        }
    }
    return TdcChannelPtr();
}

TdcChannelPtr TdcService::RemoveChannel(const std::string& name) {
    auto it = channels_.find(name);
    if (it == channels_.end()) {
        return nullptr;
    }
    auto channel = it->second;
    channels_.erase(it);
    return channel;
}

void TdcService::SendMsg(const std::string &name, const std::string &body, TdcMessageCallback callback) {
    auto channel = GetChannel(name);
    if (channel) {
        channel->SendMsg(body, std::move(callback));
    }
}

void TdcService::SendMsg(const std::string& name, const void* body, size_t len, TdcMessageCallback callback) {
    auto channel = GetChannel(name);
    if (channel) {
        channel->SendMsg(body, len, std::move(callback));
    }
}

void TdcService::SendMsg(const std::string& name, TdcMessagePtr msg) {
    auto channel = GetChannel(name);
    if (channel) {
        channel->SendMsg(std::move(msg));
    }
}

void TdcService::RegisterService() {
    resolver_->Put(address_.first, address_.second, static_cast<uint32_t>(options_.expiryTime), [this](const naming::NamingReply& reply) {
        if (reply.err) {
            if (failed_count_ % kNameRegistrationFailedWarningThreshold == 0) {
                log_warning("Service register failed %llu times: name[%s], value[%s], err[%d], msg[%s]",
                            (failed_count_ + 1), address_.first.c_str(), address_.second.c_str(), reply.err, tinynet_strerror(reply.err));
            }
            ++failed_count_;
            success_count_ = 0;
            return;
        }
        if (success_count_ % kNameRegistrationSuccessloggingThreshold == 0) {
            log_info("Service register success: name[%s], value[%s]", address_.first.c_str(), address_.second.c_str());
        }
        ++success_count_;
        failed_count_ = 0;
    });
}

void TdcService::AfterSend(int64_t channel_guid, int64_t msg_guid) {
    auto channel = GetChannel(channel_guid);
    if (channel) {
        channel->AfterSend(msg_guid);
        return;
    }
    log_warning("Can not find channel[%lld], maybe removed!", channel_guid);
}

void TdcService::AfterResolved(int64_t channel_guid, const naming::NamingReply& reply) {
    auto channel = GetChannel(channel_guid);
    if (channel) {
        channel->AfterResolved(reply);
        return;
    }
    log_warning("Can not find channel[%lld], maybe removed!", channel_guid);
}

bool TdcService::ParseMessage(const std::string& msg_body) {
    if (receive_msg_cb_) {
        return receive_msg_cb_(msg_body);
    }
    return false;
}

void TdcService::Stop() {
    if (register_timer_) {
        event_loop()->ClearTimer(register_timer_);
    }
    if (resolver_) {
        resolver_->Stop();
    }
    if (channels_.size() > 0) {
        for (auto channel : channels_) {
            channel.second->Run(ERROR_RPC_REQUESTCANCELED);
        }
    }
    if (server_) {
        server_->Stop();
    }
}
}
}
