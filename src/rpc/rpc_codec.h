// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include "rpc_packet.h"
#include <net/stream_socket.h>
#include <memory>
namespace tinynet {
namespace rpc {
class RpcCodec {
  public:
    void Write(net::SocketPtr& sock, RpcPacket *packet);

    void Write(net::SocketPtr& sock, PacketType type, uint64_t seq, uint64_t method, const google::protobuf::Message *msg);

    PacketHeader* Read(net::SocketPtr& sock);

    google::protobuf::Message* Read(net::SocketPtr& sock, google::protobuf::Message* body, size_t len);
  public:
    PacketHeader header_;
};
}
}
