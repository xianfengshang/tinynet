// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "tdc.pb.h"
#include <memory>
namespace tinynet {
namespace tdc {
class TdcRpcServiceImpl;
typedef std::shared_ptr<TdcRpcServiceImpl> TdcRpcServiceImplPtr;
class TdcService;
class TdcRpcServiceImpl :
    public TdcRpcService {
  public:
    TdcRpcServiceImpl(TdcService *service);
  public:
    void Transfer(::google::protobuf::RpcController* controller,
                  const ::tinynet::tdc::TransferRequest* request,
                  ::tinynet::tdc::TransferResponse* response,
                  ::google::protobuf::Closure* done) override;
  private:
    TdcService * service_;
};
}
}
