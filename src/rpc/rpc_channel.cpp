// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "rpc_channel.h"
#include "rpc_server.h"
#include "logging/logging.h"
#include "base/unique_id.h"
#include "base/error_code.h"
#include "util/uri_utils.h"
#include "net/stream_socket.h"

namespace tinynet {
namespace rpc {

static int kRpcChannelTimeout = 10 * 1000;

RpcChannel::RpcChannel(net::SocketPtr sock, RpcServer *server) :
    net::SocketChannel(sock, server),
    seq_(0),
    codec_(new (std::nothrow)RpcCodec()) {
}

RpcChannel::RpcChannel(EventLoop *loop) :
    net::SocketChannel(loop),
    seq_(0),
    codec_(new (std::nothrow)RpcCodec()) {
}

RpcChannel::~RpcChannel() = default;

void RpcChannel::Init(const std::string &ip, int port) {
    net::ChannelOptions opts;
    opts.host = ip;
    opts.port = port;
    Init(opts);
}

void RpcChannel::Init(net::ChannelOptions& opts) {
    opts.timeout = opts.timeout ? opts.timeout : kRpcChannelTimeout;
    if (opts.name.empty()) opts_.name = "RPC";
    SocketChannel::Init(opts);
}

void RpcChannel::Reset() {
    PACKET_QUEUE empty;
    packet_queue_.swap(empty);
    Close(0);
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method, google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done) {
    ++seq_;
    rpc_map_[seq_] = std::make_shared<RpcInfo>(seq_, controller, request, response, done);
    SendRequest(seq_, method->full_name(), request);
}

void RpcChannel::OnOpen() {
    log_info("[RPC] RPC channel(%lld, %s) opened", guid_, address_.c_str());
    Flush();
}

void RpcChannel::OnError(int err) {
    log_info("[RPC] RPC channel(%lld, %s) closed, err:%d, msg:%s",
             guid_, address_.c_str(), err, tinynet_strerror(err));
    Run(err);
}

void RpcChannel::OnRead() {
    PacketHeader* header;
    while (socket_->is_connected() && (header = codec_->Read(socket_)) != nullptr) {
        OnPacket(header);
    }
}

void RpcChannel::OnPacket(PacketHeader* header) {
    if (header->type == (uint32_t)PacketType::REQUEST) {
        auto server = get_server<RpcServer>();
        if (!server) {
            Close(ERROR_RPC_CHANNELERROR);
            return;
        }
        auto method = server->GetMethod(header->method);
        if (!method) {
            Close(ERROR_RPC_METHODNOTFOUND);
            return;
        }
        std::unique_ptr<google::protobuf::Message> request(method->NewRequest());
        if (!codec_->Read(socket_, request.get(), header->len)) {
            Close(ERROR_RPC_DECODEERROR);
            return;
        }
        auto rpc = std::make_shared<RpcInfo>(header->seq, request.release(), method->NewResponse());

        method->get_service()->CallMethod(method->get_descriptor(),
                                          rpc->get_controller(),
                                          rpc->get_request(),
                                          rpc->get_response(),
                                          ::google::protobuf::NewCallback(server, &RpcServer::SendResponse, guid_, rpc));
        return;
    }
    auto call = PopRpc(header->seq);
    if (!call) {
        Close(ERROR_RPC_SEQUENCEERROR);
        return;
    }
    auto response = call->get_response();
    if (!codec_->Read(socket_, response, header->len)) {
        Close(ERROR_RPC_DECODEERROR);
        return;
    }
    call->Run(ERROR_OK);
}

bool RpcChannel::SendRequest(int64_t seq, const std::string& method, const google::protobuf::Message* request) {
    switch (get_state()) {
    case net::ChannelState::CS_CLOSED: //Reset => BeginConnect => Enqueue
        Reset();
    case net::ChannelState::CS_UNSPEC: //BeginConnect => Enqueue
        Open();
    case net::ChannelState::CS_CONNECTING: {
        packet_queue_.emplace(std::move(std::make_shared<RpcPacket>(seq, request, StringUtils::Hash3(method.c_str()))));
        break;
    }
    case net::ChannelState::CS_CONNECTED: {
        codec_->Write(socket_, PacketType::REQUEST, seq, StringUtils::Hash3(method.c_str()), request);
        break;
    }
    default:
        break;
    }
    return true;
}

void RpcChannel::PushRpc(uint64_t seq, RpcInfoPtr rpc) {
    rpc_map_.emplace(seq, std::move(rpc));
}

RpcInfoPtr RpcChannel::PopRpc(uint64_t seq) {
    RpcInfoPtr rpc;
    auto it = rpc_map_.find(seq);
    if (it != rpc_map_.end()) {
        rpc = std::move(it->second);
        rpc_map_.erase(it);
    }
    return rpc;
}

void RpcChannel::SendResponse(RpcInfoPtr rpc) {
    codec_->Write(socket_, PacketType::RESPONSE, rpc->get_seq(), 0, rpc->get_response());
}

void RpcChannel::Run(int err) {
    RPC_MAP rpc_map;
    rpc_map_.swap(rpc_map);
    for (auto it = rpc_map.begin(); it != rpc_map.end();) {
        it->second->Run(err);
        rpc_map.erase(it++);
    }
}

void RpcChannel::Flush() {
    while (!packet_queue_.empty()) {
        codec_->Write(socket_, packet_queue_.front().get());
        packet_queue_.pop();
    }
}

}
}
