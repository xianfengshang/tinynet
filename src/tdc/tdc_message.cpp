// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tdc_message.h"
namespace tinynet {
namespace tdc {
TdcMessage::TdcMessage(int64_t guid,
                       const std::string &body,
                       TdcMessageCallback callback) :
    callback_(std::move(callback)) {
    request_.set_guid(guid);
    request_.set_body(body);
}

TdcMessage::TdcMessage(int64_t guid,
                       const void* body,
                       size_t len,
                       TdcMessageCallback callback) :
    callback_(std::move(callback)) {
    request_.set_guid(guid);
    request_.set_body(body, len);
}

TdcMessage::TdcMessage(int64_t guid,
                       const tinynet::iovs_t& iovs,
                       TdcMessageCallback callback) :
    callback_(std::move(callback)) {
    request_.set_guid(guid);
    std::string* body = request_.mutable_body();
    for (auto& iov : iovs) {
        body->append(iov.base, iov.len);
    }
}

void TdcMessage::Run(int32_t err) {
    if (callback_) {
        callback_(err);
        callback_ = nullptr;
    }
}

void TdcMessage::Send(TdcRpcService_Stub *stub, ::google::protobuf::Closure *done) {
    controller_.Reset();
    return stub->Transfer(&controller_, &request_, &response_, done);
}

}
}
