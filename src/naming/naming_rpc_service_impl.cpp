// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "naming_rpc_service_impl.h"
#include "rpc/rpc_helper.h"
#include "naming_service.h"
#include "naming_state.h"
#include "base/error_code.h"
namespace tinynet {
namespace naming {
NamingRpcServiceImpl::NamingRpcServiceImpl(NamingService * service) :
    service_(service) {
}

void NamingRpcServiceImpl::Invoke(::google::protobuf::RpcController* controller, const ::tinynet::naming::ClientRequest* request, ::tinynet::naming::ClientResponse* response, ::google::protobuf::Closure* done) {
    auto store = service_->get_state();
    if (!store) {
        rpc::ClosureGuard done_gurad(done);
        response->set_error_code(ERROR_TNS_SERVICEUNAVAILABLE);
        return;
    }
    auto opcode = request->opcode();
    switch (opcode) {
    case GET_REQ: {
        store->Get(controller, request, response, done);
        return;
    }
    case PUT_REQ: {
        store->Put(controller, request, response, done);
        return;
    }
    case DEL_REQ: {
        store->Delete(controller, request, response, done);
        return;
    }
    case KEYS_REQ: {
        store->Keys(controller, request, response, done);
        return;
    }
    default: {
        rpc::ClosureGuard done_gurad(done);
        response->set_error_code(ERROR_TNS_METHODNOTFOUND);
        return;
    }

    }
}
}
}
