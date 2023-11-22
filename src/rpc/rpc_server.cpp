// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "rpc_server.h"
#include "rpc_channel.h"
#include "logging/logging.h"
#include "util/string_utils.h"
#include <functional>
#include "base/error_code.h"
namespace tinynet {
namespace rpc {

RpcServer::RpcServer(EventLoop *loop) :
    SocketServer(loop) {
}

RpcServer::~RpcServer() = default;

int RpcServer::Start(const std::string &ip, int port) {
    net::ServerOptions opts;
    opts.listen_ip = ip;
    opts.listen_port = port;

    return Start(opts);
}

int RpcServer::Start(const std::tuple<int, int>& port) {
    net::ServerOptions opts;
    opts.listen_ports = port;
    return Start(opts);
}

int RpcServer::Start(const std::string& unix_path) {
    net::ServerOptions opts;
    opts.listen_path = unix_path;
    return Start(opts);
}

int RpcServer::Start(net::ServerOptions& opts) {
    if (opts.name.empty()) opts.name = "RPC";
    return SocketServer::Start(opts);
}

uint64_t RpcServer::Now() {
    return event_loop_->Time();
}

net::SocketChannelPtr RpcServer::CreateChannel(net::SocketPtr sock) {
    return std::make_shared<RpcChannel>(std::move(sock), this);
}

void RpcServer::RegisterService(std::shared_ptr<google::protobuf::Service> service) {
    const google::protobuf::ServiceDescriptor* serviceDescriptor = service->GetDescriptor();
    for (int i = 0; i < serviceDescriptor->method_count(); ++i) {
        const google::protobuf::MethodDescriptor *methodDescriptor = serviceDescriptor->method(i);
        uint64_t method_id = StringUtils::Hash3(methodDescriptor->full_name().c_str());
        methods_[method_id] = std::make_shared<RpcMethod>(service, methodDescriptor);
    }
}
RpcMethodPtr RpcServer::GetMethod(uint64_t method_id) {
    auto it = methods_.find(method_id);
    if (it == methods_.end()) {
        return nullptr;
    }
    return it->second;
}

void RpcServer::SendResponse(int64_t guid, RpcInfoPtr rpc) {
    auto channel = GetChannel<RpcChannel>(guid);
    if (channel) {
        channel->SendResponse(std::move(rpc));
    }
}
}
}
