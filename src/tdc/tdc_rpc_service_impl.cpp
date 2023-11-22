// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "tdc_rpc_service_impl.h"
#include "rpc/rpc_helper.h"
#include "base/error_code.h"
#include "tdc_service.h"
#include <typeinfo>
#include <utility>
#include <util/string_utils.h>

namespace tinynet {
namespace tdc {
TdcRpcServiceImpl::TdcRpcServiceImpl(TdcService *service) :
    service_(service) {

}
void TdcRpcServiceImpl::Transfer(::google::protobuf::RpcController* controller,
                                 const ::tinynet::tdc::TransferRequest* request,
                                 ::tinynet::tdc::TransferResponse* response,
                                 ::google::protobuf::Closure* done) {
    rpc::ClosureGuard done_guard(done);
    response->set_guid(request->guid());
    service_->ParseMessage(request->body());
    response->set_error_code(ERROR_OK);
}
}
}
