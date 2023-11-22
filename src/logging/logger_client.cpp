// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "logger_client.h"
#include "util/string_utils.h"
#include "util/process_utils.h"
#include "base/unique_id.h"
#include "net/stream_socket.h"
#include "base/runtime_logger.h"
#include "base/error_code.h"
#include <functional>
#include "google/protobuf/stubs/common.h"
#include "base/clock.h"
#include "util/net_utils.h"
#include "logging.pb.h"

namespace tinynet {
namespace logging {

LoggerClient::LoggerClient(EventLoop *loop) :
    event_loop_(loop) {

}
LoggerClient::~LoggerClient() = default;

void LoggerClient::Init(const std::string& name ) {
    name_ = name;
    channel_.reset(CreateChannel(name));
    stub_.reset(new(std::nothrow) LoggingRpcService_Stub(channel_.get()));
}

void LoggerClient::Stop() {
}

rpc::RpcChannel* LoggerClient::CreateChannel(const std::string& name) {
    auto channel = new(std::nothrow) rpc::RpcChannel(event_loop_);
    net::ChannelOptions options;
    options.name = "Log";
    options.path = name;
    channel->Init(options);
    return channel;
}

void LoggerClient::Log(LoggingContextPtr ctx ) {
    if (!ctx) return;
    ctx->controller.Reset();
    stub_->Log(&ctx->controller, &ctx->request, &ctx->response,
               ::google::protobuf::NewCallback(this, &LoggerClient::AfterLog, ctx));
}

void LoggerClient::AfterLog(LoggingContextPtr ctx) {
    if (!ctx || !ctx->callback) return;

    if (ctx->controller.Failed()) {
        ctx->callback(ctx->controller.ErrorCode());
        return;
    }
    if (ctx->response.error_code() != ERROR_OK) {
        ctx->callback(ctx->response.error_code());
        return;
    }
    ctx->callback(ctx->response.error_code());
}
}
}
