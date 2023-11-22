// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cstdint>
#include <string>
#include "tdc.pb.h"
#include <memory>
#include <functional>
#include "rpc/rpc_controller.h"
#include "base/io_buffer.h"

namespace tinynet {
namespace tdc {
typedef std::function<void(int32_t err)> TdcMessageCallback;

class TdcMessage;
typedef std::shared_ptr<TdcMessage> TdcMessagePtr;

class TdcMessage {
  public:
    TdcMessage(int64_t guid, const std::string& body, TdcMessageCallback callback);
    TdcMessage(int64_t guid, const void* body, size_t len, TdcMessageCallback callback);
    TdcMessage(int64_t guid, const tinynet::iovs_t& iovs, TdcMessageCallback callback);
  public:
    void Run(int32_t err);
    void Send(TdcRpcService_Stub* stub, ::google::protobuf::Closure* done);
  public:
    const tdc::TransferRequest& get_request() const {
        return request_;
    }
    const tdc::TransferResponse& get_response() const {
        return response_;
    }
    const rpc::RpcController& get_controller() const {
        return controller_;
    }
    int64_t get_guid() const {
        return request_.guid();
    }
  protected:
    tdc::TransferRequest request_;
    tdc::TransferResponse response_;
    rpc::RpcController controller_;
    TdcMessageCallback callback_;
};
}
}
