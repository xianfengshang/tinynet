// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "google/protobuf/service.h"
#include "net/socket_channel.h"
#include <queue>
#include <memory>
#include "rpc_packet.h"
#include "rpc_codec.h"
#include <unordered_map>
#include "rpc_controller.h"
#include "rpc_method.h"
#include "rpc_info.h"

namespace tinynet {
namespace rpc {
class RpcServer;
class RpcChannel;
typedef std::shared_ptr<RpcChannel> RpcChannelPtr;

/**
 * @brief RPC channel
 *
 */
class RpcChannel :
    public net::SocketChannel,
    public google::protobuf::RpcChannel {
  public:
    /**
     * @brief Construct a new Rpc Channel object
     *
     * @param sock
     * @param server
     */
    RpcChannel(net::SocketPtr sock, RpcServer *server);
    /**
     * @brief Construct a new Rpc Channel object
     *
     * @param loop
     */
    RpcChannel(EventLoop *loop);
    /**
     * @brief Destroy the Rpc Channel object
     *
     */
    virtual ~RpcChannel();

  public:
    /**
     * @brief Initialize the rpc channel with the given ip and port
     *
     * @param ip
     * @param port
     */
    void Init(const std::string &ip, int port);

    /**
     * @brief Initialize the rpc channel with the given options
     *
     * @param opts
     */
    void Init(net::ChannelOptions &opts) override;

    /**
     * @brief Reset the rpc channel
     *
     */
    void Reset();

  public:
    /**
     * @brief Override google protobuf rpc channel CallMethod
     *
     * @param method
     * @param controller
     * @param request
     * @param response
     * @param done
     */
    virtual void CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done) override;

  public:
    void SendResponse(RpcInfoPtr rpc);
  public:
    void Run(int err);
  protected:
    void OnOpen() override;
    void OnRead() override;
    void OnError(int err) override;
    void OnPacket(PacketHeader *header);
  private:
    bool SendRequest(int64_t seq, const std::string &method, const google::protobuf::Message *request);
    void PushRpc(uint64_t seq, RpcInfoPtr rpc);
    RpcInfoPtr PopRpc(uint64_t seq);
    void Flush();

  private:
    using RPC_MAP = std::unordered_map<uint64_t, RpcInfoPtr>;
    using PACKET_QUEUE = std::queue<RpcPacketPtr>;
    using RpcCodecPtr = std::unique_ptr<RpcCodec>;
  private:
    uint64_t                  seq_;
    PACKET_QUEUE              packet_queue_;
    RpcCodecPtr               codec_;
    RPC_MAP                   rpc_map_;
};
}
}
