// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include "google/protobuf/message.h"
#include "util/string_utils.h"

namespace tinynet {
namespace rpc {

/*
RPC fragment header
0				1				2				3
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+---------+-------------------------------+--------------+
|					packet type(4/4 bytes)					   |
+-+-+-+---------+-------------------------------+--------------+
|					payload len(4/4 bytes)					   |
+-+-+-+---------+-------------------------------+--------------+
|					method(4/8bytes)						   |
+-+-+-+---------+-------------------------------+--------------+
|					method(4/8bytes)						   |
+-+-+-+---------+-------------------------------+--------------+
|					seq(4/8bytes)							   |
+-+-+-+---------+-------------------------------+--------------+
|					seq(4/8bytes)							   |
+-+-+-+---------+-------------------------------+--------------+
|					payload data							   |
+-+-+-+---------+-------------------------------+--------------+
|					payload data continued					   |
+-+-+-+---------+-------------------------------+--------------+
*/
struct RpcPacket;
typedef std::shared_ptr<RpcPacket> RpcPacketPtr;

enum class PacketType: uint32_t {
    REQUEST,
    RESPONSE
};

struct PacketHeader {
    uint32_t type ; //type field indicates the packet whether a request packet or a response packet.
    uint32_t len;
    uint64_t method;
    uint64_t seq;
};

constexpr size_t RPC_PACKET_HEADER_LEN = 24;

constexpr size_t MAX_RPC_PACKET_LEN = 256 * 1024 * 1024; //256M

struct RpcPacket {
    PacketHeader header;
    std::string body;

    RpcPacket() = default;
    RpcPacket(uint64_t seq,const google::protobuf::Message *msg, uint64_t method = 0, PacketType type = PacketType::REQUEST) {
        header.seq = seq;
        header.type = (uint32_t)type;
        header.method = method;
        msg->SerializeToString(&body);
        header.len = (uint32_t)body.length();
    }

};
}
}