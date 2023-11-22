// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include "google/protobuf/message.h"
#include "google/protobuf/service.h"

namespace tinynet {
namespace rpc {
class RpcInfo;
typedef std::shared_ptr<RpcInfo> RpcInfoPtr;

class RpcInfo {
  public:
    RpcInfo(uint64_t seq, google::protobuf::RpcController *controller, const google::protobuf::Message *request,
            google::protobuf::Message *response, google::protobuf::Closure *done);
    RpcInfo(uint64_t seq, const google::protobuf::Message* request, google::protobuf::Message *response);
    ~RpcInfo();
  public:
    uint64_t get_seq() { return seq_; }

    google::protobuf::RpcController* get_controller() { return controller_; }

    const google::protobuf::Message* get_request() { return request_; }

    google::protobuf::Message* get_response() { return response_; }

    template<class T>
    const T* get_request() { return static_cast<T*>(request_); }

    template<class T>
    T* get_response() { return static_cast<T*>(response_); }
  public:
    void Run(int err);
  private:
    uint64_t seq_;
    google::protobuf::RpcController *controller_;
    const google::protobuf::Message *request_;
    google::protobuf::Message *response_;
    google::protobuf::Closure *done_;
    bool delete_members_;
};
}
}
