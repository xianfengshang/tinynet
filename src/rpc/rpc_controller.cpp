// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "rpc_controller.h"
#include "base/error_code.h"
#include "base/clock.h"
#include "logging/logging.h"
namespace tinynet {
namespace rpc {

void RpcController::Reset() {
    error_code_ = ERROR_OK;
    canceled_ = false;
    cancel_event_handler_ = nullptr;
    send_time_ = Time_ms();
}

bool RpcController::Failed() const {
    return error_code_ != ERROR_OK;
}

std::string RpcController::ErrorText() const {
    return tinynet_strerror(error_code_);
}

void RpcController::StartCancel() {
    canceled_ = true;
    if (cancel_event_handler_) {
        cancel_event_handler_->Run();
    }
}

void RpcController::SetFailed(const std::string& reason) {
}

bool RpcController::IsCanceled() const {
    return canceled_;
}

void RpcController::NotifyOnCancel(google::protobuf::Closure* callback) {
    cancel_event_handler_ = callback;
}
void RpcController::Trace() {
    log_info("Rpc response time %lld", (Time_ms() - send_time_));
}
}
}