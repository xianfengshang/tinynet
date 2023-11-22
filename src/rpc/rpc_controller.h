// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "google/protobuf/service.h"
namespace tinynet {
namespace rpc {
class RpcController :
    public google::protobuf::RpcController {
  public:
    //@google::protobuf::RpcController
    virtual void Reset() override;
    virtual bool Failed() const override;
    virtual std::string ErrorText() const override;
    virtual void StartCancel() override;
    virtual void SetFailed(const std::string& reason) override;
    virtual bool IsCanceled() const override;
    virtual void NotifyOnCancel(google::protobuf::Closure* callback) override;
  public:
    void SetFailed(int err) { error_code_ = err; }

    int ErrorCode() const { return error_code_; }

  public:
    void Trace();
  private:
    int error_code_;
    bool canceled_{false};
    google::protobuf::Closure *cancel_event_handler_{nullptr};
    int64_t send_time_;
};
}
}