// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tdc_channel.h"
#include "net/stream_socket.h"
#include "tdc_service.h"
#include "base/error_code.h"
#include "logging/logging.h"
#include "util/string_utils.h"
#include "base/unique_id.h"
#include "google/protobuf/stubs/common.h"
#include "util/net_utils.h"
#include "util/uri_utils.h"
#include <algorithm>

namespace tinynet {
namespace tdc {

static const uint32_t kMaxSendWindowSize = 16;
static const uint32_t kInitSendWindowSize = 1;
static const int kMessageQueueOverflowThreshold = 100000;
static const int kMaxRetryCount = 3;
static const int kMaxRetryDelay = 10 * 1000;

TdcChannel::TdcChannel(const std::string& name, TdcService *service) :
    guid_(NewUniqueId()),
    name_(name),
    service_(service),
    state_(CS_INIT),
    send_window_(kInitSendWindowSize),
    retry_count(0),
    error_code_(0),
    timer_guid_(0) {
    channel_ = std::make_shared<rpc::RpcChannel>(service_->event_loop());
    stub_ = std::make_shared <TdcRpcService_Stub>(channel_.get());
}

TdcChannel::~TdcChannel() {
    if (timer_guid_) {
        service_->event_loop()->ClearTimer(timer_guid_);
    }
}

void TdcChannel::Init() {
}

void TdcChannel::SendMsg(const std::string &body, TdcMessageCallback callback) {
    SendMsg(std::make_shared<TdcMessage>(service_->event_loop()->NewUniqueId(), body, std::move(callback)));
}

void TdcChannel::SendMsg(const void* body, size_t len, TdcMessageCallback callback) {
    SendMsg(std::make_shared<TdcMessage>(service_->event_loop()->NewUniqueId(), body, len, std::move(callback)));
}

void TdcChannel::SendMsg(TdcMessagePtr msg) {
    send_queue_.Emplace(std::move(msg));
    if (send_queue_.Size() >= kMessageQueueOverflowThreshold) {
        HandleError(ERROR_TDC_MESSAGEQUEUEOVERFLOW);
        return;
    }
    Update();
}

void TdcChannel::HandleError(int err) {
    log_info("TDC channel(%llu, %s) error occurred, err:%d, msg:%s",
             guid_, name_.c_str(), err, tinynet_strerror(err));
    error_code_ = err;
    state_ = CS_INIT;
    channel_->Reset();
    Run(err);
}

void TdcChannel::Send() {
    while (send_queue_.Rsize() > 0 && send_queue_.Lsize() < send_window_) {
        TdcMessagePtr msg = send_queue_.Next();
        msg->Send(stub_.get(), google::protobuf::NewCallback(service_, &TdcService::AfterSend, guid_, msg->get_guid()));
    }
}

void TdcChannel::Resend(int err) {
    if (retry_count >= kMaxRetryCount) {
        HandleError(err);
        return;
    }
    if (timer_guid_) return;
    ++retry_count;
    uint64_t delay = (std::min)(retry_count * retry_count * 1000, kMaxRetryDelay);
    state_ = CS_INIT;

    timer_guid_ = service_->event_loop()->AddTimer(delay, 0, std::bind(&TdcChannel::UpdateLater, this));
}

void TdcChannel::AfterSend(int64_t msg_guid) {
    if (send_queue_.Lsize() == 0) {
        if (state_ != CS_INIT) {
            log_error("TDC channel(%lld, %s) no msg has been sent.",
                      guid_, name_.c_str());
        }
        return;
    }
    TdcMessagePtr msg = send_queue_.Front();
    if (msg->get_guid() != msg_guid) {
        HandleError(ERROR_TDC_MESSAGEOUTOFSEQUENCE);
        return;
    }
    int err = msg->get_controller().ErrorCode();
    if (err) {
        HandleError(err);
        return;
    }
    if (msg->get_response().guid() != msg_guid) {
        log_warning("TDC channel(%lld, %s) equest msg guid %lld not equal response msg guid %lld",
                    guid_, name_.c_str(), msg_guid, msg->get_response().guid());
    }
    err = msg->get_response().error_code();

    if (err == ERROR_TDC_SERVICEMOVED) {
        send_queue_.Rewind();
        Resend(err);
        return;
    }
    send_queue_.Pop();
    msg->Run(err);
    send_window_ = (std::min)(send_window_ * 2, kMaxSendWindowSize);
    retry_count = 0;
    Send();
}

void TdcChannel::Update() {
    switch (state_) {
    case CS_INIT:
        Resolve();
        break;
    case CS_RESOLVING:
        break;
    case CS_RESOLVED:
        Send();
        break;
    default:
        break;
    }
}

void TdcChannel::UpdateLater() {
    timer_guid_ = 0;
    Update();
}

void TdcChannel::Resolve() {
    std::string address_name;
    std::string* resolving_name;
    if (StringUtils::StartsWith(name_, service_->get_root_dir())) {
        resolving_name = &name_;
    } else {
        address_name.append(service_->get_root_dir()).append(name_);
        resolving_name = &address_name;
    }
    log_info("TDC channel(%lld, %s) resolving name:%s", guid_, name_.c_str(), resolving_name->c_str());
    service_->get_resolver()->Get(*resolving_name, std::bind(&TdcService::AfterResolved, service_, guid_, std::placeholders::_1));
    state_ = CS_RESOLVING;
}

void TdcChannel::AfterResolved(const naming::NamingReply& reply) {
    if (reply.err) {
        log_info("TDC channel(%lld, %s) resolved name error, err:%d, msg:%s",
                 guid_,  name_.c_str(), reply.err, tinynet_strerror(reply.err));
        HandleError(reply.err);
        return;
    }
    if (!UriUtils::parse_address(reply.value, &host_, &port_)) {
        HandleError(ERROR_TNS_UNRECOGNIZEDFORMAT);
        return;
    }
    log_info("TDC channel(%lld, %s) resolved name success, address:%s",
             guid_, name_.c_str(), reply.value.c_str());
    net::ChannelOptions opts;
    opts.name = name_;
    opts.host = host_;
    opts.port = port_;
    opts.debug = true;
    channel_->Init(opts);
    state_ = CS_RESOLVED;
    Update();
}

void TdcChannel::Run(int err) {
    send_queue_.Run(err);
}

}
}
