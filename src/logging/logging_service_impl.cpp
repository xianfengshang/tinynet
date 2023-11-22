// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "logging_service_impl.h"
#include "rpc/rpc_helper.h"
#include "base/error_code.h"
#include "logging_service.h"
#include <typeinfo>
#include <utility>
#include "util/string_utils.h"
#include "logging.h"
#include "base/runtime_logger.h"

namespace tinynet {
namespace logging {
LoggingServiceImpl::LoggingServiceImpl(LoggingService *service) :
    service_(service) {
}
void LoggingServiceImpl::Log(::google::protobuf::RpcController* controller, const ::tinynet::logging::LogReq* request, ::tinynet::logging::LogResp* response, ::google::protobuf::Closure* done) {
    rpc::ClosureGuard done_guard(done);
    if (!request) {
        response->set_error_code(ERROR_INVAL);
        return;
    }
    response->set_error_code(ERROR_OK);
    service_->Log(*request);
}
}
}
