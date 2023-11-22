// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "rpc_packet.h"
#include "rpc_method.h"
#include "rpc_channel.h"
#include "net/stream_listener.h"
#include "net/socket_server.h"
#include <unordered_map>
#include <unordered_set>

namespace tinynet {
namespace rpc {
// RPC server
class RpcServer:
    public net::SocketServer {
  public:
    RpcServer(EventLoop *loop);
    virtual ~RpcServer();
  public:
    //Start the rpc server and bind it to the given ip and port
    int Start(const std::string &ip, int port);
    //Start the rpc server and try bind it to a free port in the given port range
    int Start(const std::tuple<int, int>& port);
    //Start the rpc server and bind it to the given path
    int Start(const std::string& unix_path);

    int Start(net::ServerOptions& opts) override;

    virtual net::SocketChannelPtr CreateChannel(net::SocketPtr sock) override;
    //virtual RpcChannelPtr CreateChannel(const std::string& ip, int port);
    uint64_t Now();
  public:
    //Register a service
    void RegisterService(std::shared_ptr<google::protobuf::Service> service);
  public:
    using METHOD_MAP = std::unordered_map<uint64_t, RpcMethodPtr>;
    const METHOD_MAP& get_methods() { return methods_; }
    RpcMethodPtr GetMethod(uint64_t method_id);
    void SendResponse(int64_t guid, RpcInfoPtr rpc);
  private:
    using CHANNEL_MAP = std::unordered_map<int64_t, RpcChannelPtr>;
    using CHANNEL_ID_SET = std::unordered_set<int64_t>;
  private:
    METHOD_MAP			methods_;
    net::ServerOptions	opts_;
};
}
}
